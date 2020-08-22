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

#include "VolatileKeyValueStore.h"
#include <iostream>

using namespace std;
using namespace nlohmann;

static constexpr size_t kSetPartitionIndex = 0;
static constexpr size_t kGetPartitionIndex = 1;

static const string kSetPartitionName{"set"};
static const string kGetPartitionName{"get"};

namespace maplang {

using StorageMap = unordered_map<string, Packet>;

class Setter : public INode, public ISink {
 public:
  Setter(const shared_ptr<StorageMap>& storage, const string& keyName,
         bool retainBuffers)
      : mKeyName(keyName), mRetainBuffers(retainBuffers) {}

  ~Setter() override = default;

  IPathable* asPathable() override { return nullptr; }
  ISource* asSource() override { return nullptr; }
  ISink* asSink() override { return this; }

  void handlePacket(const Packet* incomingPacket) override {
    const json& key = incomingPacket->parameters[mKeyName];
    if (key.is_null() || !key.is_string()) {
      return;
    }

    vector<Buffer> storeBuffers;
    if (mRetainBuffers) {
      storeBuffers = incomingPacket->buffers;
    }

    (*mStorage).emplace(
        pair<string, Packet>(
            key.get<string>(),
            {incomingPacket->parameters, move(storeBuffers)}));//[key.get<string>()] = move(toStore);
  }

 private:
  const string mKeyName;
  const bool mRetainBuffers;
  const shared_ptr<StorageMap> mStorage;
};

class Getter : public INode, public IPathable {
 public:
  Getter(const shared_ptr<const StorageMap>& storage, const string& keyName)
      : mKeyName(keyName) {}

  ~Getter() override = default;

  IPathable* asPathable() override { return this; }
  ISource* asSource() override { return nullptr; }
  ISink* asSink() override { return nullptr; }

  void handlePacket(const PathablePacket* packet) override {
    const string key = packet->parameters[mKeyName].get<string>();

    auto it = mStorage->find(key);
    if (it == mStorage->end()) {
      Packet notFoundPacket;
      notFoundPacket.parameters["keyNotPresent"] = key;
      packet->packetPusher->pushPacket(&notFoundPacket, "keyNotFound");
      return;
    }

    const Packet& result = it->second;
    packet->packetPusher->pushPacket(&result, "gotValue");
  }

 private:
  const string mKeyName;
  const shared_ptr<const StorageMap> mStorage;
};

VolatileKeyValueStore::VolatileKeyValueStore(
    const nlohmann::json& initParameters) {

  cout << initParameters << endl;
  if (!initParameters.contains("key")) {
    throw runtime_error("VolatileKeyValueStore parameters must contain 'key'.");
  }
  const string keyName = initParameters["key"].get<string>();

  bool retainBuffers = false;
  if (initParameters.contains("retainBuffers")) {
    retainBuffers = initParameters["retainBuffers"].get<bool>();
  }

  shared_ptr<unordered_map<string, Packet>> storage =
      make_shared<unordered_map<string, Packet>>();

  mPartitions[kSetPartitionName].name = kSetPartitionName;
  mPartitions[kSetPartitionName].node =
      make_shared<Setter>(storage, keyName, retainBuffers);

  auto getter = make_shared<Getter>(storage, keyName);
  mPartitions[kGetPartitionName].name = kGetPartitionName;
  mPartitions[kGetPartitionName].node = getter;
}

size_t VolatileKeyValueStore::getNodeCount() { return mPartitions.size(); }

string VolatileKeyValueStore::getNodeName(size_t partitionIndex) {
  switch (partitionIndex) {
    case kSetPartitionIndex:
      return kSetPartitionName;
    case kGetPartitionIndex:
      return kGetPartitionName;
    default:
      return "";
  }
}

std::shared_ptr<INode> VolatileKeyValueStore::getNode(
    const string& partitionName) {
  return mPartitions[partitionName].node;
}

}  // namespace maplang
