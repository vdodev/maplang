/*
 * Copyright 2020 VDO Dev Inc <support@maplang.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "nodes/BufferAccumulatorNode.h"

#include "maplang/BufferFactory.h"
#include "maplang/stream-util.h"

using namespace std;

#define DEFINE_ACCUMULATOR_CLASS(CLASS_NAME__)                          \
  class CLASS_NAME__ final : public IImplementation, public IPathable { \
   public:                                                              \
    CLASS_NAME__(                                                       \
        const shared_ptr<const IBufferFactory>& bufferFactory,                \
        const shared_ptr<vector<BufferInfo>>& buffers)                  \
        : mBufferFactory(bufferFactory), mBuffers(buffers) {}           \
                                                                        \
    void handlePacket(const maplang::PathablePacket& packet) override;  \
                                                                        \
    maplang::ISource* asSource() override { return nullptr; }           \
    maplang::IPathable* asPathable() override { return this; }          \
    maplang::IGroup* asGroup() override { return nullptr; }             \
                                                                        \
   private:                                                             \
    const shared_ptr<const IBufferFactory> mBufferFactory;                    \
    const shared_ptr<vector<BufferInfo>> mBuffers;                      \
  }

namespace maplang {

struct BufferInfo {
  Buffer buffer;
  size_t usedByteCount = 0;
};

const std::string BufferAccumulatorNode::kChannel_AccumulatedBuffersReady =
    "Buffers Ready";

const std::string BufferAccumulatorNode::kNodeName_AppendBuffers =
    "Append Buffers";
const std::string BufferAccumulatorNode::kNodeName_SendAccumulatedBuffers =
    "Send Accumulated Buffers";
const std::string BufferAccumulatorNode::kNodeName_ClearBuffers =
    "Clear Buffers";

DEFINE_ACCUMULATOR_CLASS(AppendBuffers);
DEFINE_ACCUMULATOR_CLASS(SendAccumulatedBuffers);
DEFINE_ACCUMULATOR_CLASS(ClearBuffers);

void AppendBuffers::handlePacket(const maplang::PathablePacket& packet) {
  const auto& incomingBuffers = packet.packet.buffers;
  for (size_t i = 0; i < incomingBuffers.size(); i++) {
    const Buffer& incomingBuffer = incomingBuffers[i];

    if (mBuffers->size() == i) {
      BufferInfo bufferInfo;
      bufferInfo.buffer = mBufferFactory->Create(incomingBuffer.length);
      bufferInfo.usedByteCount = 0;

      mBuffers->push_back(std::move(bufferInfo));
    }

    BufferInfo& bufferInfo = mBuffers->at(i);
    const size_t copyToBufferAtOffset = bufferInfo.usedByteCount;
    const size_t minimumCopyToBufferLength =
        copyToBufferAtOffset + incomingBuffer.length;

    if (bufferInfo.buffer.length < minimumCopyToBufferLength) {
      Buffer newBuffer = mBufferFactory->Create(minimumCopyToBufferLength);
      memcpy(
          newBuffer.data.get(),
          bufferInfo.buffer.data.get(),
          bufferInfo.buffer.length);

      std::swap(bufferInfo.buffer, newBuffer);
    }

    memcpy(
        bufferInfo.buffer.data.get() + copyToBufferAtOffset,
        incomingBuffer.data.get(),
        incomingBuffer.length);

    bufferInfo.usedByteCount += incomingBuffer.length;
  }
}

void SendAccumulatedBuffers::handlePacket(
    const maplang::PathablePacket& incomingPacket) {
  Packet outgoingPacket;

  for (size_t i = 0; i < mBuffers->size(); i++) {
    const BufferInfo& bufferInfo = mBuffers->at(i);
    const Buffer& copyFromBuffer = bufferInfo.buffer;

    outgoingPacket.buffers.push_back(
        mBufferFactory->Create(bufferInfo.usedByteCount));

    memcpy(
        outgoingPacket.buffers.rbegin()->data.get(),
        copyFromBuffer.data.get(),
        bufferInfo.usedByteCount);
  }

  incomingPacket.packetPusher->pushPacket(
      std::move(outgoingPacket),
      BufferAccumulatorNode::kChannel_AccumulatedBuffersReady);
}

void ClearBuffers::handlePacket(const maplang::PathablePacket& packet) {
  for (size_t i = 0; i < mBuffers->size(); i++) {
    BufferInfo& bufferInfo = mBuffers->at(i);

    bufferInfo.usedByteCount = 0;
  }
}

BufferAccumulatorNode::BufferAccumulatorNode(
    const Factories& factories,
    const nlohmann::json& initData)
    : mFactories(factories) {
  const auto bufferFactory = factories.bufferFactory;
  const auto buffers = make_shared<vector<BufferInfo>>();

  mInterfaces[kNodeName_AppendBuffers] =
      make_shared<AppendBuffers>(bufferFactory, buffers);
  mInterfaces[kNodeName_SendAccumulatedBuffers] =
      make_shared<SendAccumulatedBuffers>(bufferFactory, buffers);
  mInterfaces[kNodeName_ClearBuffers] =
      make_shared<ClearBuffers>(bufferFactory, buffers);
}

size_t BufferAccumulatorNode::getInterfaceCount() { return mInterfaces.size(); }

std::string BufferAccumulatorNode::getInterfaceName(size_t interfaceIndex) {
  size_t index = 0;
  for (const auto& pair : mInterfaces) {
    if (interfaceIndex == index) {
      return pair.first;
    }
  }

  THROW("Interface index " << interfaceIndex << " is out of bounds");
}

std::shared_ptr<IImplementation> BufferAccumulatorNode::getInterface(
    const std::string& interfaceName) {
  return mInterfaces[interfaceName];
}

}  // namespace maplang
