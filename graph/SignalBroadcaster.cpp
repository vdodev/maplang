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

#include "SignalBroadcaster.h"

using namespace std;
using namespace nlohmann;

namespace dgraph {
/*
static list<shared_ptr<GraphElement>> findGraphTails(const Graph& graph) {
  list<shared_ptr<GraphElement>> graphTailElements;
  graph.getGraphElements([&graphTailElements](const shared_ptr<GraphElement>& element){
    if (element->forwardEdges.empty()) {
      graphTailElements.push_back(element);
    }
  });

  return graphTailElements;
}
*/
class SignalBroadcastContext final : public ISignalBroadcastContext {
 public:
  SignalBroadcastContext(const weak_ptr<DataGraphImpl>& graphImpl) : mGraphImpl(graphImpl) {}
  ~SignalBroadcastContext() override = default;

  void broadcastSignal(const json& parameters);

  bool isBroadcastPaused() const { return mBroadcastPaused; }
  void holdBroadcast() override { mBroadcastPaused = true; }

  void resumeBroadcast() override {
    /*
    const shared_ptr<DataGraphImpl> graphImpl = mGraphImpl.lock();

    Packet resumePacket;
    graphImpl->sendPacket(&resumePacket, this);
     */
  }

 private:
  const weak_ptr<DataGraphImpl> mGraphImpl;
  bool mBroadcastPaused = false;

};

SignalBroadcaster::SignalBroadcaster(
    stack<shared_ptr<GraphElement>>&& graphElementsToSignal, const json& parameters)
: mGraphElementsToSignal(move(graphElementsToSignal)), mParameters(parameters) {}

stack<shared_ptr<GraphElement>> SignalBroadcaster::createStackOfElementsForBroadcast(
    const list<shared_ptr<GraphElement>>& startingFromElements) const {

  unordered_map<const void*, bool> queued;
  stack<shared_ptr<GraphElement>> elementStack;

  for (const auto& startElement : startingFromElements) {
    elementStack.push(startElement);
    queued[startElement.get()] = true;
  }

  while (!mGraphElementsToSignal.empty()) {
    auto top = mGraphElementsToSignal.top();

    for (const auto& nodePointingAtUs : top->backEdges) {
      bool prevWasQueued = queued[nodePointingAtUs.get()];

      if (!prevWasQueued) {
        elementStack.push(nodePointingAtUs);
        queued[nodePointingAtUs.get()] = true;
      }
    }
  }

  return elementStack;
}

void SignalBroadcaster::broadcastSignal(
    list<shared_ptr<GraphElement>>&& startingFromElements, const json& parameters) {

}

void SignalBroadcaster::continueBroadcast() {
  /*
  const auto strongThis = shared_from_this();

  if (mGraphElementsToSignal.empty()) {
    return;
  }

  mContext->mPaused = false;
  auto top = mGraphElementsToSignal.top();
  mGraphElementsToSignal.pop();

  if (!mSignalled[top.get()]) {
    mSignalled[top.get()] = true;
    const shared_ptr<INode> nodeToSignal = top->node;
    nodeToSignal->sendSignal(mContext, mParameters);
  }

  if (!mContext->isBroadcastPaused() && !mGraphElementsToSignal.empty()) {
    scheduleContinueBroadcast();
    return;
  }
   */
}

void SignalBroadcaster::scheduleContinueBroadcast() {
  /*
  Packet continuePacket;
  const shared_ptr<DataGraphImpl> graphImpl = mGraphImpl.lock();
  if (!graphImpl) {
    return;
  }

  graphImpl->sendPacket(&continuePacket, shared_from_this());
   */
}

}  // namespace dgraph
