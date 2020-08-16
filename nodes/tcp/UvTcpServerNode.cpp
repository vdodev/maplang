/*
 * Copyright 2020 VDO Dev Inc <support@maplang.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <sstream>
#include <list>
#include <uv.h>

#include "UvTcpServerNode.h"
#include "maplang/ObjectPool.h"

using namespace std;
using namespace nlohmann;

namespace maplang {

static const string kChannel_DataReceived = "Data Received";
static const string kChannel_Listening = "Listening";
static const string kChannel_NewConnection = "New Connection";
static const string kChannel_ConnectionClosed = "Connection Closed";
static const string kChannel_Error = "Error";
static const string kChannel_DataQueued = "Data Queued";
static const string kChannel_LocallyDisconnected = "Locally Disconnected";
static const string kChannel_RemotelyDisconnected = "Remotely Disconnected";

static const string kParameter_TcpConnectionId = "TcpConnectionId";
static const string kParameter_Address = "Address";
static const string kParameter_LocalAddress = "LocalAddress";
static const string kParameter_RemoteAddress = "RemoteAddress";
static const string kParameter_Port = "Port";
static const string kParameter_LocalPort = "LocalPort";
static const string kParameter_RemotePort = "RemotePort";
static const string kParameter_ErrorMessage = "ErrorMessage";
static const string kParameter_Backlog = "NewConnectionBacklog";
static const string kParameter_NoDelay = "NoDelay";
static const string kParameter_ClosedReason = "ClosedReason";

static const string kClosedReason_StreamEnded = "StreamEnded";

static const string kNodeName_Sender = "Sender";
static const string kNodeName_Receiver = "Receiver";
static const string kNodeName_Listener = "Listener";
static const string kNodeName_Disconnector = "Disconnector";


struct UvTcpConnection final {
  shared_ptr<uv_tcp_t> uvSocket;
  string connectionId;
  string closedReason;
  string remoteAddress;
  uint16_t remotePort;
};

static void sendErrorPacket(const string& message, int status, const string& connectionId, const shared_ptr<IPacketPusher>& pusher) {
  Packet packet;
  packet.parameters[kParameter_TcpConnectionId] = connectionId;

  static constexpr size_t kErrorNameLength = 128;
  char errorName[kErrorNameLength];
  errorName[0] = 0;
  uv_strerror_r(status, errorName, kErrorNameLength);
  errorName[kErrorNameLength - 1] = 0;

  ostringstream messageStream;
  messageStream << message << " " << errorName << " (" << status << ").";
  packet.parameters[kParameter_ErrorMessage] = messageStream.str();

  pusher->pushPacket(&packet, kChannel_Error);
}

static int getSocketAddressAndPort(const sockaddr_storage* sock, string* outAddress, uint16_t* outPort, const shared_ptr<IPacketPusher>& pusherForErrors) {
  string address;
  uint16_t port;
  int status;
  if (sock->ss_family == AF_INET6) {
    const auto ip6Sock = reinterpret_cast<const sockaddr_in6*>(sock);
    port = ntohs(ip6Sock->sin6_port);

    char ip6AddrString[INET6_ADDRSTRLEN];
    status = uv_ip6_name(ip6Sock, ip6AddrString, sizeof(ip6AddrString));
    if (status != 0) {
      const string connectionId = "";
      sendErrorPacket("Could not create IPv6 address string.", status, connectionId, pusherForErrors);
      return status;
    }

    address = ip6AddrString;
  } else if (sock->ss_family == AF_INET) {
    const auto ip4Sock = reinterpret_cast<const sockaddr_in*>(sock);
    port = ntohs(ip4Sock->sin_port);

    char ip4AddrString[INET_ADDRSTRLEN];
    status = uv_ip4_name(ip4Sock, ip4AddrString, sizeof(ip4AddrString));
    if (status != 0) {
      const string connectionId = "";
      sendErrorPacket("Could not create IPv4 address string.", status, connectionId, pusherForErrors);
      return status;
    }

    address = ip4AddrString;
  } else {
    const string connectionId = "";
    ostringstream errorMessageStream;
    errorMessageStream << "Unexpected address family " << sock->ss_family << ".";
    sendErrorPacket(errorMessageStream.str(), EINVAL, connectionId, pusherForErrors);
    return status;
  }

  const string ipv4Wrapper = "::ffff:";
  if (address.find(ipv4Wrapper) == 0) {
    address = address.substr(ipv4Wrapper.length());
  }

  *outAddress = address;
  *outPort = port;

  return 0;
}

static constexpr size_t kBufferSizeInPool = 64 * 1024;

struct ExtendedUvWriteT {
  uv_write_t uvWriteRequest;
  Buffer buffer;
};

class UvTcpServerImpl final {
 public:
  UvTcpServerImpl()
  : mBufferPool(
      []{ return new uint8_t[kBufferSizeInPool]; },
      [](uint8_t* buf) { delete[] buf; }),
    mUvWriteTPool(
        []{ return new ExtendedUvWriteT(); },
        [](ExtendedUvWriteT* writeReq) {
          writeReq->buffer.data.reset();
          writeReq->buffer.length = 0;
        }) {}

  ~UvTcpServerImpl() {

  }

  void listen(const Packet* packet) {
    const bool alreadyListening = !mListeningAddressPortPair.empty();
    if (alreadyListening) {
      sendErrorPacket("Already listening.", EINVAL, "", mListenerPacketPusher);
      return;
    }

    if (!packet->parameters.contains(kParameter_Port)) {
      const string connectionId = "";
      sendErrorPacket("Missing parameter '" + kParameter_Port + "'.", EINVAL, connectionId, mListenerPacketPusher);
      return;
    }

    string address = "::";
    if (packet->parameters.contains(kParameter_Address)) {
      address = packet->parameters[kParameter_Address];
    }

    int backlog = 100;
    if (packet->parameters.contains(kParameter_Backlog)) {
      backlog = packet->parameters[kParameter_Backlog].get<int32_t>();
    }

    mNoDelay = false;
    if (packet->parameters.contains(kParameter_NoDelay)) {
      mNoDelay = packet->parameters[kParameter_NoDelay].get<bool>();
    }

    uint16_t port = packet->parameters[kParameter_Port].get<uint16_t>();

    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    int status = uv_ip6_addr(address.c_str(), port, reinterpret_cast<sockaddr_in6*>(&addr));
    if (status != 0) {
      status = uv_ip4_addr(address.c_str(), port, reinterpret_cast<sockaddr_in*>(&addr));

      if (status != 0) {
        const string connectionId = "";
        sendErrorPacket("Could not parse address '" + address + "' port " + to_string(port) + ".", status, connectionId, mListenerPacketPusher);
        return;
      }
    }

    status = uv_tcp_bind(&mTcpServer, reinterpret_cast<const sockaddr*>(&addr), 0);
    if (status != 0) {
      const string connectionId = "";
      sendErrorPacket("Could not bind to address '" + address + "' port " + to_string(port) + ".", status, connectionId, mListenerPacketPusher);
      return;
    }

    mTcpServer.data = this;
    status = uv_listen(reinterpret_cast<uv_stream_t*>(&mTcpServer), backlog, onNewConnectionWrapper);
    if (status != 0) {
      const string connectionId = "";
      sendErrorPacket("Could not listen on address '" + address + "' port " + to_string(port) + ".", status, connectionId, mListenerPacketPusher);
      return;
    }

    sockaddr_storage populatedSocket;
    memset(&populatedSocket, 0, sizeof(populatedSocket));

    int socketSize = sizeof(populatedSocket);
    status = uv_tcp_getsockname(&mTcpServer, reinterpret_cast<sockaddr*>(&populatedSocket), &socketSize);
    if (status != 0) {
      const string connectionId = "";
      sendErrorPacket("Could not get listening server address/port.", status, connectionId, mListenerPacketPusher);
      return;
    }

    string boundAddress = "";
    uint16_t boundPort = 0;
    status = getSocketAddressAndPort(&populatedSocket, &boundAddress, &boundPort, mListenerPacketPusher);
    if (status != 0) {
      return;  // already sent error packet
    }

    mBoundAddress = boundAddress;
    mBoundPort = boundPort;
    mListeningAddressPortPair = mBoundAddress + ":" + to_string(mBoundPort);

    Packet listenSuccessPacket;
    listenSuccessPacket.parameters[kParameter_LocalPort] = boundPort;
    listenSuccessPacket.parameters[kParameter_LocalAddress] = boundAddress;
    mListenerPacketPusher->pushPacket(&listenSuccessPacket, kChannel_Listening);
  }

  static void onNewConnectionWrapper(uv_stream_t* server, int status) {
    auto serverImpl = reinterpret_cast<UvTcpServerImpl*>(server->data);
    serverImpl->onNewConnection(server, status);
  }

  void onNewConnection(uv_stream_t* server, int status) {
    if (status < 0) {
      const string connectionId = "";
      sendErrorPacket("New connection failed.", status, connectionId, mListenerPacketPusher);
      return;
    }

    auto client = make_shared<uv_tcp_t>();
    uv_tcp_init(mUvLoop.get(), client.get());
    client->data = this;
    status = uv_accept(server, reinterpret_cast<uv_stream_t*>(client.get()));
    if (status != 0) {
      const string connectionId = "";
      sendErrorPacket("Failed to accept connection.", status, connectionId, mListenerPacketPusher);
      return;
    }

    sockaddr_storage clientSocket;
    int clientSocketSize = sizeof(clientSocket);
    memset(&clientSocket, 0, sizeof(clientSocket));
    status = uv_tcp_getpeername(client.get(), reinterpret_cast<sockaddr*>(&clientSocket), &clientSocketSize);
    if (status != 0) {
      string connectionId = "";
      sendErrorPacket("Failed to get remote address/port from socket.", status, connectionId, mListenerPacketPusher);
      return;
    }

    string remoteAddress;
    uint16_t remotePort;
    status = getSocketAddressAndPort(&clientSocket, &remoteAddress, &remotePort, mListenerPacketPusher);
    if (status != 0) {
      return; // error packet already sent
    }

    const string connectionId = remoteAddress + ":" + to_string(remotePort);

    UvTcpConnection connection;
    connection.uvSocket = client;
    connection.connectionId = connectionId;
    connection.remoteAddress = remoteAddress;
    connection.remotePort = remotePort;

    mConnections[connectionId] = connection;
    mUvClientToConnection[client.get()] = &mConnections[connectionId];

    status = uv_read_start(reinterpret_cast<uv_stream_t*>(client.get()), allocateBufferWrapper, dataReceivedWrapper);
    if (status != 0) {
      sendErrorPacket("Failed to start reading from incoming connection.", status, connectionId, mListenerPacketPusher);
      uv_close(reinterpret_cast<uv_handle_t*>(client.get()), onConnectionRemovedWrapper);
      return;
    }

    Packet newConnectionPacket;
    setConnectionParameters(connection, &newConnectionPacket.parameters);

    mListenerPacketPusher->pushPacket(&newConnectionPacket, kChannel_NewConnection);
  }

  static void dataReceivedWrapper(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    auto serverImpl = reinterpret_cast<UvTcpServerImpl*>(stream->data);
    serverImpl->dataReceived(stream, nread, buf);
  }

  void dataReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    UvTcpConnection *const connection = mUvClientToConnection[stream];

    Buffer buffer;
    if (buf->base) {
      buffer.data = shared_ptr<uint8_t>(reinterpret_cast<uint8_t*>(buf->base), [this](uint8_t* rawBuffer) {
        mBufferPool.returnToPool(rawBuffer);
      });
    }

    if (nread == UV_EOF) {
      connection->closedReason = "End of stream";
      uv_close(reinterpret_cast<uv_handle_t*>(connection->uvSocket.get()), onConnectionRemovedWrapper);
      return;
    } else if (nread < 0) {
      sendErrorPacket("TCP receive error.", nread, connection->connectionId, mDataReceivedPacketPusher);
      return;
    }

    if (nread == 0) {
      return;
    }

    Packet dataReceivedPacket;
    setConnectionParameters(*connection, &dataReceivedPacket.parameters);

    buffer.length = nread;
    dataReceivedPacket.buffers.push_back(move(buffer));

    mDataReceivedPacketPusher->pushPacket(&dataReceivedPacket, kChannel_DataReceived);
  }

  void sendData(const Packet* packet) {
    UvTcpConnection& connection = mConnections[packet->parameters[kParameter_TcpConnectionId].get<string>()];
    const Buffer& buffer = packet->buffers[0];
    ExtendedUvWriteT* writeRequest = mUvWriteTPool.get();
    writeRequest->buffer = buffer;
    writeRequest->uvWriteRequest.data = this;

    uv_buf_t uvBuffer = uv_buf_init(reinterpret_cast<char*>(buffer.data.get()), buffer.length);
    uv_write(
        reinterpret_cast<uv_write_t*>(writeRequest),
        reinterpret_cast<uv_stream_t*>(connection.uvSocket.get()),
        &uvBuffer,
        1,
        onDataSentWrapper);
  }

  static void onDataSentWrapper(uv_write_t* req, int status) {
    auto serverImpl = reinterpret_cast<UvTcpServerImpl*>(req->data);
    serverImpl->onDataSent(req, status);
  }

  void onDataSent(uv_write_t* req, int status) {
    auto extendedRequest = reinterpret_cast<ExtendedUvWriteT*>(req);
    extendedRequest->buffer.data.reset();
    extendedRequest->buffer.length = 0;

    mUvWriteTPool.returnToPool(extendedRequest);
  }

  void disconnect(const PathablePacket* packet) {
    const string connectionId = packet->parameters[kParameter_TcpConnectionId];
    const auto it = mConnections.find(connectionId);
    if (it == mConnections.end()) {
      return;
    }

    UvTcpConnection& connection = it->second;
    connection.closedReason = "Local side requested disconnect.";

    const shared_ptr<uv_tcp_s> uvSocket = connection.uvSocket;
    uv_close(reinterpret_cast<uv_handle_t*>(uvSocket.get()), onConnectionRemovedWrapper);
  }

  void setUvLoop(const shared_ptr<uv_loop_t>& uvLoop) {
    if (mUvLoop != nullptr) {
      throw runtime_error("UV Loop cannot be set more than once.");
    }

    mUvLoop = uvLoop;
    int status = uv_tcp_init(mUvLoop.get(), &mTcpServer);
    if (status != 0) {
      const string connectionId = "";
      sendErrorPacket("Could not initialize server's TCP socket.", status, connectionId, mListenerPacketPusher);
    }
  }

  void setListenerPacketPusher(const shared_ptr<IPacketPusher>& packetPusher) {
    mListenerPacketPusher = packetPusher;
  }

  void setReceiverPacketPusher(const shared_ptr<IPacketPusher>& packetPusher) {
    mDataReceivedPacketPusher = packetPusher;
  }

 private:
  shared_ptr<uv_loop_t> mUvLoop;
  uv_tcp_t mTcpServer;
  string mListeningAddressPortPair;
  string mBoundAddress;
  uint16_t mBoundPort = 0;

  bool mNoDelay = false;
  shared_ptr<IPacketPusher> mListenerPacketPusher;
  shared_ptr<IPacketPusher> mDataReceivedPacketPusher;

  unordered_map<string, UvTcpConnection> mConnections;
  unordered_map<const void*, UvTcpConnection*> mUvClientToConnection;

  ObjectPool<uint8_t> mBufferPool;
  ObjectPool<ExtendedUvWriteT> mUvWriteTPool;

 private:
  void setConnectionParameters(const UvTcpConnection& connection, json* parameters) const {
    (*parameters)[kParameter_TcpConnectionId] = connection.connectionId;
    (*parameters)[kParameter_LocalAddress] = mBoundAddress;
    (*parameters)[kParameter_LocalPort] = mBoundPort;
    (*parameters)[kParameter_RemoteAddress] = connection.remoteAddress;
    (*parameters)[kParameter_RemotePort] = connection.remotePort;
  }

  static void allocateBufferWrapper(uv_handle_t* handle, size_t suggestedSize, uv_buf_t* buf) {
    auto serverImpl = reinterpret_cast<UvTcpServerImpl*>(handle->data);
    serverImpl->allocateBuffer(handle, suggestedSize, buf);
  }

  void allocateBuffer(uv_handle_t* handle, size_t suggestedSize, uv_buf_t* buf) {
    auto connection = mUvClientToConnection[handle];
    buf->base = reinterpret_cast<char*>(malloc(suggestedSize));
    if (!buf->base) {
      buf->len = 0;
      return;
    }

    buf->base = reinterpret_cast<char*>(mBufferPool.get());
    buf->len = kBufferSizeInPool;
  }

  static void onConnectionRemovedWrapper(uv_handle_t* handle) {
    auto serverImpl = reinterpret_cast<UvTcpServerImpl*>(handle->data);
    serverImpl->onConnectionRemoved(handle);
  }

  void onConnectionRemoved(uv_handle_t* handle) {
    auto lookupByPointerIt = mUvClientToConnection.find(handle);
    if (lookupByPointerIt == mUvClientToConnection.end()) {
      return;
    }

    const string connectionId = lookupByPointerIt->second->connectionId;
    mUvClientToConnection.erase(lookupByPointerIt);

    auto connectionIt = mConnections.find(connectionId);
    UvTcpConnection connection;
    if (connectionIt != mConnections.end()) {
      connection = connectionIt->second;
      mConnections.erase(connectionIt);
    }

    Packet closedPacket;
    closedPacket.parameters[kParameter_TcpConnectionId] = connectionId;
    closedPacket.parameters[kParameter_ClosedReason] = connection.closedReason;
    mDataReceivedPacketPusher->pushPacket(&closedPacket, kChannel_ConnectionClosed);
  }
};

class UvTcpListener : public ISink, public ISource, public INode {
 public:
  UvTcpListener(const shared_ptr<UvTcpServerImpl>& server) : mServer(server) {}
  ~UvTcpListener() override = default;

  void handlePacket(const Packet* packet) override { mServer->listen(packet); }

  IPathable *asPathable() override { return nullptr; }
  ISink *asSink() override { return this; }
  ISource *asSource() override {return this; }

  void setPacketPusher(const shared_ptr<IPacketPusher>& packetPusher) override {
    mServer->setListenerPacketPusher(packetPusher);
  }

  void setSubgraphContext(const std::shared_ptr<ISubgraphContext> &context) override {
    mServer->setUvLoop(context->getUvLoop());
  }

 private:
  const shared_ptr<UvTcpServerImpl> mServer;
};

class UvTcpDataReceiver : public ISource, public INode {
 public:
  UvTcpDataReceiver(const shared_ptr<UvTcpServerImpl>& server) : mServer(server) {}
  ~UvTcpDataReceiver() override = default;

  void setPacketPusher(const shared_ptr<IPacketPusher>& packetPusher) override {
    mServer->setReceiverPacketPusher(packetPusher);
  }

  IPathable *asPathable() override { return nullptr; }
  ISink *asSink() override { return nullptr; }
  ISource *asSource() override {return this; }

 private:
  const shared_ptr<UvTcpServerImpl> mServer;
};

class UvTcpSender : public ISink, public INode {
 public:
  UvTcpSender(const shared_ptr<UvTcpServerImpl>& server) : mServer(server) {}
  ~UvTcpSender() override = default;
  void handlePacket(const Packet* packet) override { mServer->sendData(packet); }

  IPathable *asPathable() override { return nullptr; }
  ISink *asSink() override { return this; }
  ISource *asSource() override {return nullptr; }

 private:
  const shared_ptr<UvTcpServerImpl> mServer;
};

class UvTcpDisconnector : public IPathable, public INode {
 public:
  UvTcpDisconnector(const shared_ptr<UvTcpServerImpl>& server) : mServer(server) {}
  ~UvTcpDisconnector() override = default;
  void handlePacket(const PathablePacket* packet) override { mServer->disconnect(packet); }

  IPathable *asPathable() override { return this; }
  ISink *asSink() override { return nullptr; }
  ISource *asSource() override {return nullptr; }

 private:
  const shared_ptr<UvTcpServerImpl> mServer;
};

size_t UvTcpServerNode::getNodeCount() {
  return mNodes.size();
}

UvTcpServerNode::UvTcpServerNode() : mImpl(make_shared<UvTcpServerImpl>()) {
  mNodes[kNodeName_Listener] = make_shared<UvTcpListener>(mImpl);
  mNodes[kNodeName_Receiver] = make_shared<UvTcpDataReceiver>(mImpl);
  mNodes[kNodeName_Sender] = make_shared<UvTcpSender>(mImpl);
  mNodes[kNodeName_Disconnector] = make_shared<UvTcpDisconnector>(mImpl);
}

string UvTcpServerNode::getNodeName(size_t nodeIndex) {
  switch(nodeIndex) {
    case 0: return "Listen";
    case 1: return "Accept";
    case 2: return "Data Received";
    case 3: return "Send Data";
    case 4: return "Disconnect";
    default: throw runtime_error("Invalid node index: " + to_string(nodeIndex));
  }
}

shared_ptr<INode> UvTcpServerNode::getNode(const string& nodeName) {
  return mNodes[nodeName];
}

} // namespace maplang
