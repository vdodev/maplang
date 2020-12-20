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

#ifndef MAPLANG_INCLUDE_MAPLANG_UTIL_H_
#define MAPLANG_INCLUDE_MAPLANG_UTIL_H_

#include "maplang/Packet.h"

namespace maplang {

inline Packet packetWithParameters(nlohmann::json&& parameters) {
  return Packet{move(parameters), {}};
}

inline Packet packetWithParameters(const nlohmann::json& parameters) {
  return Packet{parameters, {}};
}

}  // namespace maplang

#define ML_CREATE_GROUP_SOURCE_SINK(                                          \
    CLASS_NAME__,                                                             \
    SHARED_CLASS__,                                                           \
    HANDLE_PACKET_METHOD__,                                                   \
    PUSHER_SETTER_METHOD__)                                                   \
  class CLASS_NAME__ final : public maplang::INode,                           \
                             public maplang::ISource,                         \
                             public maplang::ISink {                          \
   public:                                                                    \
    CLASS_NAME__(const std::shared_ptr<SHARED_CLASS__>& sharedObject)         \
        : mSharedObject(sharedObject) {}                                      \
    ~CLASS_NAME__() override = default;                                       \
                                                                              \
    void setPacketPusher(const std::shared_ptr<maplang::IPacketPusher>&       \
                             packetPusher) override {                         \
      mSharedObject->PUSHER_SETTER_METHOD__(packetPusher);                    \
    }                                                                         \
                                                                              \
    void handlePacket(const maplang::Packet& packet) override {               \
      mSharedObject->HANDLE_PACKET_METHOD__(packet);                          \
    }                                                                         \
                                                                              \
    void setSubgraphContext(                                                  \
        const std::shared_ptr<maplang::ISubgraphContext>& context) override { \
      mSharedObject->setSubgraphContext(context);                             \
    }                                                                         \
                                                                              \
    maplang::ISink* asSink() override { return this; }                        \
    maplang::ISource* asSource() override { return this; }                    \
    maplang::IPathable* asPathable() override { return nullptr; }             \
    maplang::ICohesiveGroup* asGroup() override { return nullptr; }           \
                                                                              \
   private:                                                                   \
    const std::shared_ptr<SHARED_CLASS__> mSharedObject;                      \
    std::shared_ptr<maplang::IPacketPusher> mPacketPusher;                    \
  };

#define ML_CREATE_GROUP_SOURCE(                                               \
    CLASS_NAME__,                                                             \
    SHARED_CLASS__,                                                           \
    PUSHER_SETTER_METHOD__)                                                   \
  class CLASS_NAME__ final : public maplang::INode, public maplang::ISource { \
   public:                                                                    \
    CLASS_NAME__(const std::shared_ptr<SHARED_CLASS__>& sharedObject)         \
        : mSharedObject(sharedObject) {}                                      \
    ~CLASS_NAME__() override = default;                                       \
                                                                              \
    void setPacketPusher(const std::shared_ptr<maplang::IPacketPusher>&       \
                             packetPusher) override {                         \
      mSharedObject->PUSHER_SETTER_METHOD__(packetPusher);                    \
    }                                                                         \
                                                                              \
    void setSubgraphContext(                                                  \
        const std::shared_ptr<maplang::ISubgraphContext>& context) override { \
      mSharedObject->setSubgraphContext(context);                             \
    }                                                                         \
                                                                              \
    maplang::ISink* asSink() override { return nullptr; }                     \
    maplang::ISource* asSource() override { return this; }                    \
    maplang::IPathable* asPathable() override { return nullptr; }             \
    maplang::ICohesiveGroup* asGroup() override { return nullptr; }           \
                                                                              \
   private:                                                                   \
    const std::shared_ptr<SHARED_CLASS__> mSharedObject;                      \
    std::shared_ptr<maplang::IPacketPusher> mPacketPusher;                    \
  };

#define ML_CREATE_GROUP_SINK(                                                 \
    CLASS_NAME__,                                                             \
    SHARED_CLASS__,                                                           \
    HANDLE_PACKET_METHOD__)                                                   \
  class CLASS_NAME__ final : public maplang::INode, public maplang::ISink {   \
   public:                                                                    \
    CLASS_NAME__(const std::shared_ptr<SHARED_CLASS__>& sharedObject)         \
        : mSharedObject(sharedObject) {}                                      \
    ~CLASS_NAME__() override = default;                                       \
                                                                              \
    void handlePacket(const maplang::Packet& packet) override {               \
      mSharedObject->HANDLE_PACKET_METHOD__(packet);                          \
    }                                                                         \
                                                                              \
    void setSubgraphContext(                                                  \
        const std::shared_ptr<maplang::ISubgraphContext>& context) override { \
      mSharedObject->setSubgraphContext(context);                             \
    }                                                                         \
                                                                              \
    maplang::ISink* asSink() override { return this; }                        \
    maplang::ISource* asSource() override { return nullptr; }                 \
    maplang::IPathable* asPathable() override { return nullptr; }             \
    maplang::ICohesiveGroup* asGroup() override { return nullptr; }           \
                                                                              \
   private:                                                                   \
    const std::shared_ptr<SHARED_CLASS__> mSharedObject;                      \
  };

#define ML_CREATE_GROUP_PATHABLE(                                             \
    CLASS_NAME__,                                                             \
    SHARED_CLASS__,                                                           \
    HANDLE_PACKET_METHOD__)                                                   \
  class CLASS_NAME__ final : public maplang::INode,                           \
                             public maplang::IPathable {                      \
   public:                                                                    \
    CLASS_NAME__(const std::shared_ptr<SHARED_CLASS__>& sharedObject)         \
        : mSharedObject(sharedObject) {}                                      \
    ~CLASS_NAME__() override = default;                                       \
                                                                              \
    void handlePacket(const maplang::PathablePacket& packet) override {       \
      mSharedObject->HANDLE_PACKET_METHOD__(packet);                          \
    }                                                                         \
                                                                              \
    void setSubgraphContext(                                                  \
        const std::shared_ptr<maplang::ISubgraphContext>& context) override { \
      mSharedObject->setSubgraphContext(context);                             \
    }                                                                         \
                                                                              \
    maplang::ISink* asSink() override { return nullptr; }                     \
    maplang::ISource* asSource() override { return nullptr; }                 \
    maplang::IPathable* asPathable() override { return this; }                \
    maplang::ICohesiveGroup* asGroup() override { return nullptr; }           \
                                                                              \
   private:                                                                   \
    const std::shared_ptr<SHARED_CLASS__> mSharedObject;                      \
  };

#endif  // MAPLANG_INCLUDE_MAPLANG_UTIL_H_
