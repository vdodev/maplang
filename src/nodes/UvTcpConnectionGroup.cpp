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

#include "nodes/UvTcpConnectionGroup.h"

#include <uv.h>

#include <atomic>
#include <list>
#include <mutex>
#include <sstream>

#include "Cleanup.h"
#include "maplang/Errors.h"
#include "maplang/ObjectPool.h"

using namespace std;
using namespace nlohmann;

namespace maplang {

static const string kChannel_DataReceived = "Data Received";
static const string kChannel_Listening = "Listening";
static const string kChannel_NewIncomingConnection = "New Incoming Connection";
static const string kChannel_ConnectionEstablished = "Connection Established";
static const string kChannel_ConnectionClosed = "Connection Closed";
static const string kChannel_SenderShutdown = "Sender Shutdown";
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
static const string kParameter_Backlog = "NewConnectionBacklog";
static const string kParameter_NoDelay = "NoDelay";
static const string kParameter_ClosedReason = "Closed Reason";

static const string kClosedReason_StreamEnded = "Stream Ended";

static const string kNodeName_Sender = "Sender";
static const string kNodeName_Receiver = "Receiver";
static const string kNodeName_Listener = "Listener";
static const string kNodeName_AsyncEvents = "Async Events";
static const string kNodeName_Connector = "Connector";
static const string kNodeName_Disconnector = "Disconnector";
static const string kNodeName_ShutdownSender = "Shutdown Sender";

struct UvTcpConnection final {
  shared_ptr<uv_tcp_t> uvSocket;
  string connectionId;
  string closedReason;
  string localAddress;
  uint16_t localPort;
  string remoteAddress;
  uint16_t remotePort;
};

static void sendUvErrorPacket(
    const string& message,
    int status,
    const string& connectionId,
    const shared_ptr<IPacketPusher>& pusher) {
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

  pusher->pushPacket(move(packet), kChannel_Error);
}

static int getSocketAddressAndPort(
    const sockaddr_storage* sock,
    string* outAddress,
    uint16_t* outPort,
    const shared_ptr<IPacketPusher>& pusherForErrors) {
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
      sendUvErrorPacket(
          "Could not create IPv6 address string.",
          status,
          connectionId,
          pusherForErrors);
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
      sendUvErrorPacket(
          "Could not create IPv4 address string.",
          status,
          connectionId,
          pusherForErrors);
      return status;
    }

    address = ip4AddrString;
  } else {
    const string connectionId = "";
    ostringstream errorMessageStream;
    errorMessageStream << "Unexpected address family " << sock->ss_family
                       << ".";
    sendUvErrorPacket(
        errorMessageStream.str(),
        EINVAL,
        connectionId,
        pusherForErrors);
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

static int getLocalUvAddressAndPort(
    const uv_tcp_t* tcp,
    const shared_ptr<IPacketPusher> packetPusher,
    string* boundAddress,
    uint16_t* boundPort) {
  sockaddr_storage populatedSocket;
  memset(&populatedSocket, 0, sizeof(populatedSocket));

  int socketSize = sizeof(populatedSocket);
  int status = uv_tcp_getsockname(
      tcp,
      reinterpret_cast<sockaddr*>(&populatedSocket),
      &socketSize);
  if (status != 0) {
    const string connectionId = "";
    sendUvErrorPacket(
        "Could not get local address/port.",
        status,
        connectionId,
        packetPusher);
    return status;
  }

  status = getSocketAddressAndPort(
      &populatedSocket,
      boundAddress,
      boundPort,
      packetPusher);
  if (status != 0) {
    return status;  // already sent error packet
  }

  return 0;
}

static int getRemoteUvAddressAndPort(
    const uv_tcp_t* tcp,
    const shared_ptr<IPacketPusher> packetPusher,
    string* remoteAddress,
    uint16_t* remotePort) {
  sockaddr_storage populatedSocket;
  memset(&populatedSocket, 0, sizeof(populatedSocket));

  int socketSize = sizeof(populatedSocket);
  int status = uv_tcp_getpeername(
      tcp,
      reinterpret_cast<sockaddr*>(&populatedSocket),
      &socketSize);
  if (status != 0) {
    const string connectionId = "";
    sendUvErrorPacket(
        "Could not get remote address/port.",
        status,
        connectionId,
        packetPusher);
    return status;
  }

  status = getSocketAddressAndPort(
      &populatedSocket,
      remoteAddress,
      remotePort,
      packetPusher);
  if (status != 0) {
    return status;  // already sent error packet
  }

  return 0;
}

static constexpr size_t kBufferSizeInPool = 64 * 1024;

struct ExtendedUvWriteT {
  uv_write_t uvWriteRequest;
  Buffer buffer;
};

struct ExtendedShutdownT {
  uv_shutdown_t uvShutdownRequest;
  shared_ptr<IPacketPusher> packetPusher;
};

struct ConnectInfo {
  UvTcpImpl* tcpImpl;
  shared_ptr<IPacketPusher> packetPusher;
};

class UvTcpImpl final {
 public:
  UvTcpImpl()
      : mBufferPool(
          [] { return new uint8_t[kBufferSizeInPool]; },
          [](uint8_t* buf) { delete[] buf; }),
        mUvWriteTPool(
            [] { return new ExtendedUvWriteT(); },
            [](ExtendedUvWriteT* writeReq) {
              writeReq->buffer.data.reset();
              writeReq->buffer.length = 0;
            }) {
    call_once(initAtomicConnectionIndexOnce, []() {
      atomicConnectionIndex.store(0);
    });
  }

  ~UvTcpImpl() {}

  void connect(const PathablePacket& pathablePacket) {
    const auto packet = pathablePacket.packet;

    if (!packet.parameters.contains(kParameter_Port)) {
      const string connectionId = "";
      sendUvErrorPacket(
          "Missing parameter '" + kParameter_Port + "'.",
          EINVAL,
          connectionId,
          pathablePacket.packetPusher);
      return;
    }

    if (!packet.parameters.contains(kParameter_Address)) {
      const string connectionId = "";
      sendUvErrorPacket(
          "Missing parameter '" + kParameter_Address + "'.",
          EINVAL,
          connectionId,
          pathablePacket.packetPusher);
      return;
    }

    uint16_t port = packet.parameters[kParameter_Port].get<uint16_t>();
    string address = packet.parameters[kParameter_Address].get<string>();

    bool noDelay = false;
    if (packet.parameters.contains(kParameter_NoDelay)) {
      noDelay = packet.parameters[kParameter_NoDelay].get<bool>();
    }

    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    int status = uv_ip6_addr(
        address.c_str(),
        port,
        reinterpret_cast<sockaddr_in6*>(&addr));
    if (status != 0) {
      status = uv_ip4_addr(
          address.c_str(),
          port,
          reinterpret_cast<sockaddr_in*>(&addr));

      if (status != 0) {
        const string connectionId = "";
        sendUvErrorPacket(
            "Could not parse address '" + address + "' port " + to_string(port)
                + ".",
            status,
            connectionId,
            pathablePacket.packetPusher);
        return;
      }
    }

    uv_tcp_t* uvSocket =
        reinterpret_cast<uv_tcp_t*>(calloc(1, sizeof(uv_tcp_t)));
    if (!uvSocket) {
      throw bad_alloc();
    }

    Cleanup uvSocketCleanup([uvSocket]() { free(uvSocket); });

    status = uv_tcp_init(mUvLoop.get(), uvSocket);
    if (status != 0) {
      const string connectionId = "";
      sendUvErrorPacket(
          "Could not initialize TCP client.",
          status,
          connectionId,
          pathablePacket.packetPusher);
      return;
    }

    if (noDelay) {
      uv_tcp_nodelay(uvSocket, true);
    }

    uv_connect_t* connect =
        reinterpret_cast<uv_connect_t*>(calloc(1, sizeof(uv_connect_t)));
    if (!connect) {
      throw bad_alloc();
    }

    Cleanup connectCleanup([connect]() { free(connect); });

    ConnectInfo* connectInfo = new ConnectInfo();
    connectInfo->tcpImpl = this;
    connectInfo->packetPusher = pathablePacket.packetPusher;
    connect->data = connectInfo;

    status = uv_tcp_connect(
        connect,
        uvSocket,
        reinterpret_cast<const sockaddr*>(&addr),
        onOutgoingConnectionEstablishedWrapper);
    if (status != 0) {
      const string connectionId = "";
      sendUvErrorPacket(
          "Connection failed.",
          status,
          connectionId,
          pathablePacket.packetPusher);
      return;
    }

    uvSocketCleanup.cancelCleanup();
    connectCleanup.cancelCleanup();
  }

  void listen(const PathablePacket& pathablePacket) {
    const auto& packet = pathablePacket.packet;
    const auto packetPusher = pathablePacket.packetPusher;

    const bool alreadyListening = !mListeningAddressPortPair.empty();
    if (alreadyListening) {
      sendUvErrorPacket("Already listening.", EINVAL, "", packetPusher);
      return;
    }

    if (!packet.parameters.contains(kParameter_Port)) {
      const string connectionId = "";
      sendUvErrorPacket(
          "Missing parameter '" + kParameter_Port + "'.",
          EINVAL,
          connectionId,
          packetPusher);
      return;
    }

    string address = "::";
    if (packet.parameters.contains(kParameter_Address)) {
      address = packet.parameters[kParameter_Address].get<string>();
    }

    int backlog = 100;
    if (packet.parameters.contains(kParameter_Backlog)) {
      backlog = packet.parameters[kParameter_Backlog].get<int32_t>();
    }

    mIncomingConnectionsNoDelay = false;
    if (packet.parameters.contains(kParameter_NoDelay)) {
      mIncomingConnectionsNoDelay =
          packet.parameters[kParameter_NoDelay].get<bool>();
    }

    uint16_t port = packet.parameters[kParameter_Port].get<uint16_t>();

    sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    int status = uv_ip6_addr(
        address.c_str(),
        port,
        reinterpret_cast<sockaddr_in6*>(&addr));
    if (status != 0) {
      status = uv_ip4_addr(
          address.c_str(),
          port,
          reinterpret_cast<sockaddr_in*>(&addr));

      if (status != 0) {
        const string connectionId = "";
        sendUvErrorPacket(
            "Could not parse address '" + address + "' port " + to_string(port)
                + ".",
            status,
            connectionId,
            packetPusher);
        return;
      }
    }

    status =
        uv_tcp_bind(&mTcpServer, reinterpret_cast<const sockaddr*>(&addr), 0);
    if (status != 0) {
      const string connectionId = "";
      sendUvErrorPacket(
          "Could not bind to address '" + address + "' port " + to_string(port)
              + ".",
          status,
          connectionId,
          packetPusher);
      return;
    }

    mTcpServer.data = this;
    status = uv_listen(
        reinterpret_cast<uv_stream_t*>(&mTcpServer),
        backlog,
        onNewIncomingConnectionWrapper);
    if (status != 0) {
      const string connectionId = "";
      sendUvErrorPacket(
          "Could not listen on address '" + address + "' port "
              + to_string(port) + ".",
          status,
          connectionId,
          packetPusher);
      return;
    }

    string boundAddress;
    uint16_t boundPort;
    status = getLocalUvAddressAndPort(
        &mTcpServer,
        packetPusher,
        &boundAddress,
        &boundPort);
    if (status != 0) {
      return;
    }

    mListeningAddressPortPair = boundAddress + ":" + to_string(boundPort);

    Packet listenSuccessPacket;
    listenSuccessPacket.parameters[kParameter_LocalPort] = boundPort;
    listenSuccessPacket.parameters[kParameter_LocalAddress] = boundAddress;
    packetPusher->pushPacket(move(listenSuccessPacket), kChannel_Listening);

    printf("Listening on port %s:%hu\n", boundAddress.c_str(), boundPort);
  }

  static void onOutgoingConnectionEstablishedWrapper(
      uv_connect_t* client,
      int status) {
    auto connectInfo = reinterpret_cast<ConnectInfo*>(client->data);
    const auto tcpImpl = connectInfo->tcpImpl;
    const auto packetPusher = connectInfo->packetPusher;
    delete connectInfo;

    tcpImpl->onOutgoingConnectionEstablished(packetPusher, client, status);
  }

  void onOutgoingConnectionEstablished(
      const shared_ptr<IPacketPusher>& packetPusher,
      uv_connect_t* connect,
      int status) {
    UvTcpConnection connection;
    connection.uvSocket = shared_ptr<uv_tcp_t>(
        reinterpret_cast<uv_tcp_t*>(connect->handle),
        [connect](uv_tcp_t* tcp) {
          uv_close(
              reinterpret_cast<uv_handle_t*>(tcp),
              onConnectionRemovedWrapper);

          free(tcp);
          free(connect);
        });

    if (status != 0) {
      sendUvErrorPacket(
          "Outgoing connection failed.",
          status,
          "",
          packetPusher);
      free(connect);
      return;
    }

    string remoteAddress;
    uint16_t remotePort = 0;
    status = getRemoteUvAddressAndPort(
        connection.uvSocket.get(),
        packetPusher,
        &remoteAddress,
        &remotePort);
    if (status != 0) {
      return;  // error packet already sent
    }

    string localAddress;
    uint16_t localPort = 0;
    status = getLocalUvAddressAndPort(
        connection.uvSocket.get(),
        packetPusher,
        &localAddress,
        &localPort);
    if (status != 0) {
      return;  // error packet already sent
    }

    connection.connectionId =
        remoteAddress + ":" + to_string(remotePort) + " "
        + to_string(atomic_fetch_add(&atomicConnectionIndex, 1));
    connection.localAddress = localAddress;
    connection.localPort = localPort;
    connection.remoteAddress = remoteAddress;
    connection.remotePort = remotePort;

    mConnections[connection.connectionId] = connection;
    mUvClientToConnection[connection.uvSocket.get()] =
        &mConnections[connection.connectionId];

    Packet connectedPacket;
    setConnectionParameters(connection, &connectedPacket.parameters);
    packetPusher->pushPacket(
        move(connectedPacket),
        kChannel_ConnectionEstablished);
  }

  static void onNewIncomingConnectionWrapper(uv_stream_t* server, int status) {
    auto tcpImpl = reinterpret_cast<UvTcpImpl*>(server->data);
    tcpImpl->onNewIncomingConnection(server, status);
  }

  void onNewIncomingConnection(uv_stream_t* server, int status) {
    if (status < 0) {
      const string connectionId = "";
      sendUvErrorPacket(
          "New connection failed.",
          status,
          connectionId,
          mAsyncEventsPacketPusher);
      return;
    }

    auto client = make_shared<uv_tcp_t>();
    uv_tcp_init(mUvLoop.get(), client.get());
    client->data = this;
    status = uv_accept(server, reinterpret_cast<uv_stream_t*>(client.get()));
    if (status != 0) {
      const string connectionId = "";
      sendUvErrorPacket(
          "Failed to accept connection.",
          status,
          connectionId,
          mAsyncEventsPacketPusher);
      return;
    }

    if (mIncomingConnectionsNoDelay) {
      uv_tcp_nodelay(client.get(), true);
    }

    string remoteAddress;
    uint16_t remotePort = 0;
    status = getRemoteUvAddressAndPort(
        client.get(),
        mAsyncEventsPacketPusher,
        &remoteAddress,
        &remotePort);
    if (status != 0) {
      return;  // error packet already sent
    }

    string localAddress;
    uint16_t localPort = 0;
    status = getLocalUvAddressAndPort(
        client.get(),
        mAsyncEventsPacketPusher,
        &localAddress,
        &localPort);
    if (status != 0) {
      return;  // error packet already sent
    }

    const string connectionId =
        remoteAddress + ":" + to_string(remotePort) + " "
        + to_string(atomic_fetch_add(&atomicConnectionIndex, 1));

    UvTcpConnection connection;
    connection.uvSocket = client;
    connection.connectionId = connectionId;
    connection.localAddress = localAddress;
    connection.localPort = localPort;
    connection.remoteAddress = remoteAddress;
    connection.remotePort = remotePort;

    mConnections[connectionId] = connection;
    mUvClientToConnection[client.get()] = &mConnections[connectionId];

    status = uv_read_start(
        reinterpret_cast<uv_stream_t*>(client.get()),
        allocateBufferWrapper,
        dataReceivedWrapper);
    if (status != 0) {
      sendUvErrorPacket(
          "Failed to start reading from incoming connection.",
          status,
          connectionId,
          mAsyncEventsPacketPusher);
      uv_close(
          reinterpret_cast<uv_handle_t*>(client.get()),
          onConnectionRemovedWrapper);
      return;
    }

    Packet newConnectionPacket;
    setConnectionParameters(connection, &newConnectionPacket.parameters);

    mAsyncEventsPacketPusher->pushPacket(
        move(newConnectionPacket),
        kChannel_NewIncomingConnection);
  }

  static void
  dataReceivedWrapper(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    auto tcpImpl = reinterpret_cast<UvTcpImpl*>(stream->data);

    tcpImpl->dataReceived(stream, nread, buf);
  }

  void dataReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    UvTcpConnection* const connection = mUvClientToConnection[stream];

    Buffer buffer;
    if (buf->base) {
      buffer.data = shared_ptr<uint8_t[]>(
          reinterpret_cast<uint8_t*>(buf->base),
          [this](uint8_t* rawBuffer) { mBufferPool.returnToPool(rawBuffer); });
    }

    if (nread == UV_EOF) {
      connection->closedReason = "End of stream";
      uv_close(
          reinterpret_cast<uv_handle_t*>(connection->uvSocket.get()),
          onConnectionRemovedWrapper);
      return;
    } else if (nread < 0) {
      sendUvErrorPacket(
          "TCP receive error.",
          nread,
          connection->connectionId,
          mDataReceivedPacketPusher);
      return;
    }

    if (nread == 0) {
      return;
    }

    Packet dataReceivedPacket;
    setConnectionParameters(*connection, &dataReceivedPacket.parameters);

    buffer.length = nread;
    dataReceivedPacket.buffers.push_back(move(buffer));

    mDataReceivedPacketPusher->pushPacket(
        move(dataReceivedPacket),
        kChannel_DataReceived);
  }

  void sendData(const PathablePacket& pathablePacket) {
    const auto& packet = pathablePacket.packet;

    UvTcpConnection& connection =
        mConnections[packet.parameters[kParameter_TcpConnectionId]
                         .get<string>()];
    const Buffer& buffer = packet.buffers[0];
    ExtendedUvWriteT* writeRequest = mUvWriteTPool.get();
    writeRequest->buffer = buffer;
    writeRequest->uvWriteRequest.data = this;

    uv_buf_t uvBuffer =
        uv_buf_init(reinterpret_cast<char*>(buffer.data.get()), buffer.length);
    uv_write(
        reinterpret_cast<uv_write_t*>(writeRequest),
        reinterpret_cast<uv_stream_t*>(connection.uvSocket.get()),
        &uvBuffer,
        1,
        onDataSentWrapper);
  }

  static void onDataSentWrapper(uv_write_t* req, int status) {
    auto tcpImpl = reinterpret_cast<UvTcpImpl*>(req->data);
    tcpImpl->onDataSent(req, status);
  }

  void onDataSent(uv_write_t* req, int status) {
    auto extendedRequest = reinterpret_cast<ExtendedUvWriteT*>(req);
    extendedRequest->buffer.data.reset();
    extendedRequest->buffer.length = 0;

    mUvWriteTPool.returnToPool(extendedRequest);
  }

  void disconnect(const PathablePacket& pathablePacket) {
    const auto& packet = pathablePacket.packet;

    const string connectionId = packet.parameters[kParameter_TcpConnectionId];
    const auto it = mConnections.find(connectionId);
    if (it == mConnections.end()) {
      return;
    }

    UvTcpConnection& connection = it->second;
    connection.closedReason = "Local side requested disconnect.";

    const shared_ptr<uv_tcp_s> uvSocket = connection.uvSocket;
    uv_close(
        reinterpret_cast<uv_handle_t*>(uvSocket.get()),
        onConnectionRemovedWrapper);
  }

  void shutdownSender(const PathablePacket& incomingPathablePacket) {
    const Packet& incomingPacket = incomingPathablePacket.packet;
    const string connectionId =
        incomingPacket.parameters[kParameter_TcpConnectionId];
    const auto it = mConnections.find(connectionId);
    if (it == mConnections.end()) {
      return;
    }

    UvTcpConnection& connection = it->second;
    connection.closedReason = "Local side requested disconnect.";

    const shared_ptr<uv_tcp_s> uvSocket = connection.uvSocket;
    auto shutdown = reinterpret_cast<ExtendedShutdownT*>(
        calloc(1, sizeof(ExtendedShutdownT)));
    if (!shutdown) {
      throw bad_alloc();
    }

    shutdown->packetPusher = incomingPathablePacket.packetPusher;

    int status = uv_shutdown(
        reinterpret_cast<uv_shutdown_t*>(shutdown),
        reinterpret_cast<uv_stream_t*>(connection.uvSocket.get()),
        onSenderShutdownWrapper);

    if (status != 0) {
      sendUvErrorPacket(
          "Failed to shutdown sender",
          status,
          connection.connectionId,
          incomingPathablePacket.packetPusher);
      return;
    }
  }

  void setUvLoop(const shared_ptr<uv_loop_t>& uvLoop) {
    if (mUvLoop != nullptr) {
      throw runtime_error("UV Loop cannot be set more than once.");
    }

    mUvLoop = uvLoop;
    int status = uv_tcp_init(mUvLoop.get(), &mTcpServer);
    if (status != 0) {
      const string connectionId = "";
      sendUvErrorPacket(
          "Could not initialize server's TCP socket.",
          status,
          connectionId,
          mAsyncEventsPacketPusher);
    }
  }

  void setReceiverPacketPusher(const shared_ptr<IPacketPusher>& packetPusher) {
    mDataReceivedPacketPusher = packetPusher;
  }

  void setAsyncEventsPacketPusher(
      const shared_ptr<IPacketPusher>& packetPusher) {
    mAsyncEventsPacketPusher = packetPusher;
  }

 private:
  static atomic_uint64_t atomicConnectionIndex;
  static once_flag initAtomicConnectionIndexOnce;

  shared_ptr<uv_loop_t> mUvLoop;
  uv_tcp_t mTcpServer;
  string mListeningAddressPortPair;

  bool mIncomingConnectionsNoDelay = false;
  shared_ptr<IPacketPusher> mDataReceivedPacketPusher;
  shared_ptr<IPacketPusher> mAsyncEventsPacketPusher;

  unordered_map<string, UvTcpConnection> mConnections;
  unordered_map<const void*, UvTcpConnection*> mUvClientToConnection;

  ObjectPool<uint8_t> mBufferPool;
  ObjectPool<ExtendedUvWriteT> mUvWriteTPool;

 private:
  void setConnectionParameters(
      const UvTcpConnection& connection,
      json* parameters) const {
    (*parameters)[kParameter_TcpConnectionId] = connection.connectionId;
    (*parameters)[kParameter_LocalAddress] = connection.localAddress;
    (*parameters)[kParameter_LocalPort] = connection.localPort;
    (*parameters)[kParameter_RemoteAddress] = connection.remoteAddress;
    (*parameters)[kParameter_RemotePort] = connection.remotePort;
  }

  static void allocateBufferWrapper(
      uv_handle_t* handle,
      size_t suggestedSize,
      uv_buf_t* buf) {
    auto serverImpl = reinterpret_cast<UvTcpImpl*>(handle->data);
    serverImpl->allocateBuffer(handle, suggestedSize, buf);
  }

  void
  allocateBuffer(uv_handle_t* handle, size_t suggestedSize, uv_buf_t* buf) {
    buf->base = reinterpret_cast<char*>(mBufferPool.get());
    buf->len = kBufferSizeInPool;
  }

  static void onConnectionRemovedWrapper(uv_handle_t* handle) {
    auto tcpImpl = reinterpret_cast<UvTcpImpl*>(handle->data);
    tcpImpl->onConnectionRemoved(handle);
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
    setConnectionParameters(connection, &closedPacket.parameters);
    closedPacket.parameters[kParameter_ClosedReason] = connection.closedReason;
    mAsyncEventsPacketPusher->pushPacket(
        move(closedPacket),
        kChannel_ConnectionClosed);
  }

  static void onSenderShutdownWrapper(uv_shutdown_t* shutdown, int status) {
    auto tcpImpl = reinterpret_cast<UvTcpImpl*>(shutdown->data);
    tcpImpl->onSenderShutdown(shutdown, status);
  }

  void onSenderShutdown(uv_shutdown_t* shutdown, int status) {
    ExtendedShutdownT* extendedShutdown =
        reinterpret_cast<ExtendedShutdownT*>(shutdown);
    Cleanup shutdownTCleanup([extendedShutdown]() { free(extendedShutdown); });

    auto lookupByPointerIt = mUvClientToConnection.find(shutdown->handle);
    if (lookupByPointerIt == mUvClientToConnection.end()) {
      return;
    }

    const string connectionId = lookupByPointerIt->second->connectionId;
    mUvClientToConnection.erase(lookupByPointerIt);

    if (status != 0) {
      sendUvErrorPacket(
          "Failed to shutdown sender.",
          status,
          connectionId,
          extendedShutdown->packetPusher);
      return;
    }

    auto connectionIt = mConnections.find(connectionId);
    UvTcpConnection connection;
    if (connectionIt != mConnections.end()) {
      connection = connectionIt->second;
      mConnections.erase(connectionIt);
    }

    Packet closedPacket;
    setConnectionParameters(connection, &closedPacket.parameters);
    extendedShutdown->packetPusher->pushPacket(
        move(closedPacket),
        kChannel_SenderShutdown);
  }
};

atomic_uint64_t UvTcpImpl::atomicConnectionIndex;
once_flag UvTcpImpl::initAtomicConnectionIndexOnce;

class UvTcpConnector : public IPathable, public IImplementation {
 public:
  UvTcpConnector(const shared_ptr<UvTcpImpl>& tcp) : mTcp(tcp) {}
  ~UvTcpConnector() override = default;

  void handlePacket(const PathablePacket& incomingPacket) override {
    mTcp->connect(incomingPacket);
  }

  IPathable* asPathable() override { return this; }
  ISource* asSource() override { return nullptr; }
  IGroup* asGroup() override { return nullptr; }

  void setSubgraphContext(
      const shared_ptr<ISubgraphContext>& context) override {
    mTcp->setUvLoop(context->getUvLoop());
  }

 private:
  const shared_ptr<UvTcpImpl> mTcp;
};

class UvTcpListener : public IPathable, public IImplementation {
 public:
  UvTcpListener(const shared_ptr<UvTcpImpl>& tcp) : mTcp(tcp) {}
  ~UvTcpListener() override = default;

  void handlePacket(const PathablePacket& pathablePacket) override {
    mTcp->listen(pathablePacket);
  }

  IPathable* asPathable() override { return this; }
  ISource* asSource() override { return nullptr; }
  IGroup* asGroup() override { return nullptr; }

  void setSubgraphContext(
      const shared_ptr<ISubgraphContext>& context) override {
    mTcp->setUvLoop(context->getUvLoop());
  }

 private:
  const shared_ptr<UvTcpImpl> mTcp;
};

class UvTcpSender : public IPathable, public IImplementation {
 public:
  UvTcpSender(const shared_ptr<UvTcpImpl>& tcp) : mTcp(tcp) {}
  ~UvTcpSender() override = default;
  void handlePacket(const PathablePacket& pathablePacket) override {
    mTcp->sendData(pathablePacket);
  }

  IPathable* asPathable() override { return this; }
  ISource* asSource() override { return nullptr; }
  IGroup* asGroup() override { return nullptr; }

 private:
  const shared_ptr<UvTcpImpl> mTcp;
};

class UvTcpDisconnector : public IPathable, public IImplementation {
 public:
  UvTcpDisconnector(const shared_ptr<UvTcpImpl>& tcp) : mTcp(tcp) {}
  ~UvTcpDisconnector() override = default;

  void handlePacket(const PathablePacket& pathablePacket) override {
    mTcp->disconnect(pathablePacket);
  }

  IPathable* asPathable() override { return this; }
  ISource* asSource() override { return nullptr; }
  IGroup* asGroup() override { return nullptr; }

 private:
  const shared_ptr<UvTcpImpl> mTcp;
};

class UvTcpReceiver : public ISource, public IImplementation {
 public:
  UvTcpReceiver(const shared_ptr<UvTcpImpl>& tcp) : mTcp(tcp) {}

  void setPacketPusher(const shared_ptr<IPacketPusher>& packetPusher) {
    mTcp->setReceiverPacketPusher(packetPusher);
  }

  IPathable* asPathable() override { return nullptr; }
  ISource* asSource() override { return this; }
  IGroup* asGroup() override { return nullptr; }

 private:
  const shared_ptr<UvTcpImpl> mTcp;
};

class UvTcpAsyncEvents : public ISource, public IImplementation {
 public:
  UvTcpAsyncEvents(const shared_ptr<UvTcpImpl>& tcp) : mTcp(tcp) {}

  void setPacketPusher(const shared_ptr<IPacketPusher>& packetPusher) {
    mTcp->setAsyncEventsPacketPusher(packetPusher);
  }

  IPathable* asPathable() override { return nullptr; }
  ISource* asSource() override { return this; }
  IGroup* asGroup() override { return nullptr; }

 private:
  const shared_ptr<UvTcpImpl> mTcp;
};

class UvTcpShutdownSender : public IPathable, public IImplementation {
 public:
  UvTcpShutdownSender(const shared_ptr<UvTcpImpl>& tcp) : mTcp(tcp) {}
  ~UvTcpShutdownSender() override = default;
  void handlePacket(const PathablePacket& packet) override {
    mTcp->shutdownSender(packet);
  }

  IPathable* asPathable() override { return this; }
  ISource* asSource() override { return nullptr; }
  IGroup* asGroup() override { return nullptr; }

 private:
  const shared_ptr<UvTcpImpl> mTcp;
};

UvTcpConnectionGroup::UvTcpConnectionGroup() : mImpl(make_shared<UvTcpImpl>()) {
  mInterfaces[kNodeName_Connector] = make_shared<UvTcpConnector>(mImpl);
  mInterfaces[kNodeName_Listener] = make_shared<UvTcpListener>(mImpl);
  mInterfaces[kNodeName_AsyncEvents] = make_shared<UvTcpAsyncEvents>(mImpl);
  mInterfaces[kNodeName_Sender] = make_shared<UvTcpSender>(mImpl);
  mInterfaces[kNodeName_Receiver] = make_shared<UvTcpReceiver>(mImpl);
  mInterfaces[kNodeName_Disconnector] = make_shared<UvTcpDisconnector>(mImpl);
  mInterfaces[kNodeName_ShutdownSender] =
      make_shared<UvTcpShutdownSender>(mImpl);
}

size_t UvTcpConnectionGroup::getInterfaceCount() { return mInterfaces.size(); }

string UvTcpConnectionGroup::getInterfaceName(size_t nodeIndex) {
  switch (nodeIndex) {
    case 0:
      return kNodeName_Connector;
    case 1:
      return kNodeName_Listener;
    case 2:
      return kNodeName_AsyncEvents;
    case 3:
      return kNodeName_Sender;
    case 4:
      return kNodeName_Receiver;
    case 5:
      return kNodeName_Disconnector;
    default:
      throw runtime_error("Invalid node index: " + to_string(nodeIndex));
  }
}

shared_ptr<IImplementation> UvTcpConnectionGroup::getInterface(
    const string& interfaceName) {
  const auto it = mInterfaces.find(interfaceName);
  if (it == mInterfaces.end()) {
    throw runtime_error("Interface '" + interfaceName + "' does not exist.");
  }

  return it->second;
}

}  // namespace maplang
