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

#include "VolatileKeyValueSet.h"
#include <iostream>
#include <unordered_set>

using namespace std;
using namespace nlohmann;

static constexpr size_t kSetterPartitionIndex = 0;
static constexpr size_t kGetterPartitionIndex = 1;
static constexpr size_t kRemoverPartitionIndex = 2;
static constexpr size_t kRemoveAllPartitionIndex = 3;

static const string kAdderPartitionName{"Adder"};
static const string kGetterPartitionName{"Getter"};
static const string kRemoverPartitionName{"Remover"};
static const string kRemoveAllPartitionName{"Remove All"};

static const string kParameter_KeyWhichIsNotPresent = "keyWhichIsNotPresent";
static const string kParameter_ValueWhichIsNotPresent = "valueWhichIsNotPresent";

static const string kChannel_GotValue = "Got Value";
static const string kChannel_KeyNotFound = "Key Not Found";
static const string kChannel_ValueNotFound = "Value Not Found";
static const string kChannel_RemovedValue = "Removed Value";
static const string kChannel_RemovedAllValuesForKey = "Removed All Values For Key";

namespace maplang {

using StorageMap = unordered_map<string, unordered_set<string>>;

class Adder : public INode, public IPathable {
 public:
  Adder(const shared_ptr<StorageMap>& storage, const string& keyName, const string& valueName)
      : mKeyName(keyName), mValueName(valueName) {}

  ~Adder() override = default;

  IPathable* asPathable() override { return this; }
  ISource* asSource() override { return nullptr; }
  ISink* asSink() override { return nullptr; }

  void handlePacket(const PathablePacket* incomingPacket) override {
    if (!incomingPacket->parameters.contains(mKeyName)) {
      Packet errorPacket;
      errorPacket.parameters["message"] = "Missing parameter for key-lookup: " + mKeyName;
      incomingPacket->packetPusher->pushPacket(&errorPacket, "error");
      return;
    } else if (!incomingPacket->parameters.contains(mValueName)) {
      Packet errorPacket;
      errorPacket.parameters["message"] = "Missing parameter for value-lookup: " + mValueName;
      incomingPacket->packetPusher->pushPacket(&errorPacket, "error");
      return;
    }

    const json& key = incomingPacket->parameters[mKeyName];
    if (key.is_null() || !key.is_string()) {
      return;
    }

    const string value = incomingPacket->parameters[mValueName].get<string>();

    auto it = mStorage->find(key);
    if (it == mStorage->end()) {
      mStorage->emplace(pair<string, unordered_set<string>>(key, "[]"_json));
    }

    unordered_set<string>& existingValues = mStorage->at(key);
    existingValues.emplace(move(value));

    Packet addedPacket;
    incomingPacket->packetPusher->pushPacket(&addedPacket, "added");
  }

 private:
  const string mKeyName;
  const string mValueName;
  const shared_ptr<StorageMap> mStorage;
};

class Getter : public INode, public IPathable {
 public:
  Getter(const shared_ptr<const StorageMap>& storage, const string& keyName, const string& valueName)
      : mKeyName(keyName), mValueName(valueName) {}

  ~Getter() override = default;

  IPathable* asPathable() override { return this; }
  ISource* asSource() override { return nullptr; }
  ISink* asSink() override { return nullptr; }

  void handlePacket(const PathablePacket* incomingPacket) override {
    if (!incomingPacket->parameters.contains(mKeyName)) {
      Packet errorPacket;
      errorPacket.parameters["message"] = "Missing parameter for key-lookup: " + mKeyName;
      incomingPacket->packetPusher->pushPacket(&errorPacket, "error");
      return;
    }

    const string key = incomingPacket->parameters[mKeyName].get<string>();

    auto it = mStorage->find(key);
    if (it == mStorage->end()) {
      Packet notFoundPacket;
      notFoundPacket.parameters[kParameter_KeyWhichIsNotPresent] = key;
      incomingPacket->packetPusher->pushPacket(&notFoundPacket, kChannel_ValueNotFound);
      return;
    }

    json jsonValues = "[]"_json;
    const unordered_set<string>& values = it->second;
    int i = 0;
    for (auto setIt = values.begin(); setIt != values.end(); setIt++, i++) {
      jsonValues[i] = *setIt;
    }

    Packet packetWithValues;
    packetWithValues.parameters[mKeyName] = key;
    packetWithValues.parameters[mValueName] = move(jsonValues);

    incomingPacket->packetPusher->pushPacket(&packetWithValues, kChannel_GotValue);
  }

 private:
  const string mKeyName;
  const string mValueName;
  const shared_ptr<const StorageMap> mStorage;
};

class Remover : public INode, public IPathable {
 public:
  Remover(const shared_ptr<StorageMap>& storage, const string& keyName, const string& valueName)
      : mKeyName(keyName), mValueName(valueName) {}

  ~Remover() override = default;

  IPathable* asPathable() override { return this; }
  ISource* asSource() override { return nullptr; }
  ISink* asSink() override { return nullptr; }

  void handlePacket(const PathablePacket* incomingPacket) override {
    if (!incomingPacket->parameters.contains(mKeyName)) {
      Packet errorPacket;
      errorPacket.parameters["message"] = "Missing parameter for key-lookup: " + mKeyName;
      incomingPacket->packetPusher->pushPacket(&errorPacket, "error");
      return;
    } else if (!incomingPacket->parameters.contains(mValueName)) {
      Packet errorPacket;
      errorPacket.parameters["message"] = "Missing parameter for value-lookup: " + mValueName;
      incomingPacket->packetPusher->pushPacket(&errorPacket, "error");
      return;
    }

    const string key = incomingPacket->parameters[mKeyName].get<string>();
    const string value = incomingPacket->parameters[mValueName].get<string>();

    auto it = mStorage->find(key);
    if (it == mStorage->end()) {
      Packet notFoundPacket;
      notFoundPacket.parameters[kParameter_KeyWhichIsNotPresent] = key;
      incomingPacket->packetPusher->pushPacket(&notFoundPacket, kChannel_KeyNotFound);
      return;
    }

    unordered_set<string>& values = it->second;
    auto setIt = values.find(value);
    if (setIt == values.end()) {
      Packet notFoundPacket;
      notFoundPacket.parameters[kParameter_ValueWhichIsNotPresent] = value;
      incomingPacket->packetPusher->pushPacket(&notFoundPacket, kChannel_ValueNotFound);
      return;
    }

    values.erase(setIt);

    if (values.empty()) {
      mStorage->erase(it);
    }

    Packet packetWithValues;
    packetWithValues.parameters[mKeyName] = key;
    packetWithValues.parameters[mValueName] = value;

    incomingPacket->packetPusher->pushPacket(&packetWithValues, kChannel_RemovedValue);
  }

 private:
  const string mKeyName;
  const string mValueName;
  const shared_ptr<StorageMap> mStorage;
};

class RemoveAll : public INode, public IPathable {
 public:
  RemoveAll(const shared_ptr<StorageMap>& storage, const string& keyName, const string& valueName)
      : mKeyName(keyName), mValueName(valueName) {}

  ~RemoveAll() override = default;

  IPathable* asPathable() override { return this; }
  ISource* asSource() override { return nullptr; }
  ISink* asSink() override { return nullptr; }

  void handlePacket(const PathablePacket* incomingPacket) override {
    if (!incomingPacket->parameters.contains(mKeyName)) {
      Packet errorPacket;
      errorPacket.parameters["message"] = "Missing parameter for key-lookup: " + mKeyName;
      incomingPacket->packetPusher->pushPacket(&errorPacket, "error");
      return;
    } else if (!incomingPacket->parameters.contains(mValueName)) {
      Packet errorPacket;
      errorPacket.parameters["message"] = "Missing parameter for value-lookup: " + mValueName;
      incomingPacket->packetPusher->pushPacket(&errorPacket, "error");
      return;
    }

    const string key = incomingPacket->parameters[mKeyName].get<string>();
    const string value = incomingPacket->parameters[mValueName].get<string>();

    auto it = mStorage->find(key);
    if (it == mStorage->end()) {
      Packet notFoundPacket;
      notFoundPacket.parameters[kParameter_KeyWhichIsNotPresent] = key;
      incomingPacket->packetPusher->pushPacket(&notFoundPacket, kChannel_KeyNotFound);
      return;
    }

    unordered_set<string>& values = it->second;
    json removedValuesJsonArray = "[]"_json;

    int i = 0;
    for (auto setIt = values.begin(); setIt != values.end(); setIt++, i++) {
      removedValuesJsonArray[i] = move(*setIt);
    }

    auto setIt = values.find(value);
    if (setIt == values.end()) {
      Packet notFoundPacket;
      notFoundPacket.parameters[kParameter_ValueWhichIsNotPresent] = value;
      incomingPacket->packetPusher->pushPacket(&notFoundPacket, kChannel_ValueNotFound);
      return;
    }

    Packet packetWithValues;
    packetWithValues.parameters[mKeyName] = key;
    packetWithValues.parameters[mValueName] = move(removedValuesJsonArray);

    incomingPacket->packetPusher->pushPacket(&packetWithValues, kChannel_RemovedValue);
  }

 private:
  const string mKeyName;
  const string mValueName;
  const shared_ptr<StorageMap> mStorage;
};

VolatileKeyValueSet::VolatileKeyValueSet(
    const nlohmann::json& initParameters) {

  cout << initParameters << endl;
  if (!initParameters.contains("key")) {
    throw runtime_error("VolatileKeyValueSet parameters must contain 'key'.");
  }
  const string keyName = initParameters["key"].get<string>();
  const string valueName = initParameters["value"].get<string>();

  bool retainBuffers = false;
  if (initParameters.contains("retainBuffers")) {
    retainBuffers = initParameters["retainBuffers"].get<bool>();
  }

  const auto storage = make_shared<StorageMap>();

  mPartitions[kAdderPartitionName].name = kAdderPartitionName;
  mPartitions[kAdderPartitionName].node =
      make_shared<Adder>(storage, keyName, valueName);

  auto getter = make_shared<Getter>(storage, keyName, valueName);
  mPartitions[kGetterPartitionName].name = kGetterPartitionName;
  mPartitions[kGetterPartitionName].node = getter;

  auto remover = make_shared<Remover>(storage, keyName, valueName);
  mPartitions[kRemoverPartitionName].name = kRemoverPartitionName;
  mPartitions[kRemoverPartitionName].node = remover;

  auto removeAll = make_shared<RemoveAll>(storage, keyName, valueName);
  mPartitions[kRemoveAllPartitionName].name = kRemoveAllPartitionName;
  mPartitions[kRemoveAllPartitionName].node = remover;
}

size_t VolatileKeyValueSet::getNodeCount() { return mPartitions.size(); }

string VolatileKeyValueSet::getNodeName(size_t partitionIndex) {
  switch (partitionIndex) {
    case kSetterPartitionIndex:
      return kAdderPartitionName;
    case kGetterPartitionIndex:
      return kGetterPartitionName;
    case kRemoverPartitionIndex:
      return kRemoverPartitionName;
    case kRemoveAllPartitionIndex:
      return kRemoveAllPartitionName;
    default:
      return "";
  }
}

std::shared_ptr<INode> VolatileKeyValueSet::getNode(
    const string& partitionName) {
  return mPartitions[partitionName].node;
}

}  // namespace maplang
