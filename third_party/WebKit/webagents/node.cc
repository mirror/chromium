// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/node.h"

#include "core/dom/ContainerNode.h"
#include "core/dom/Node.h"

namespace webagents {

Node::Node(blink::Node& node) : EventTarget(node) {}

blink::Node& Node::GetNode() const {
  return *GetEventTarget().ToNode();
}

base::Optional<Node> Node::parentNode() const {
  return Create<Node, blink::Node>(GetNode().parentNode());
}
base::Optional<Node> Node::firstChild() const {
  return Create<Node, blink::Node>(GetNode().firstChild());
}
base::Optional<Node> Node::nextSibling() const {
  return Create<Node, blink::Node>(GetNode().nextSibling());
}

}  // namespace webagents
