// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/node.h"

#include "third_party/WebKit/Source/core/dom/ContainerNode.h"
#include "third_party/WebKit/Source/core/dom/Node.h"

namespace webagents {

Node::Node(blink::Node* node) : EventTarget(node) {}

blink::Node* Node::GetNode() const {
  return GetEventTarget()->ToNode();
}

std::unique_ptr<Node> Create(blink::Node* node) {
  return node ? std::make_unique<Node>(node) : nullptr;
}

std::unique_ptr<Node> Node::parentNode() const {
  return Create(GetNode()->parentNode());
}
std::unique_ptr<Node> Node::firstChild() const {
  return Create(GetNode()->firstChild());
}
std::unique_ptr<Node> Node::nextSibling() const {
  return Create(GetNode()->nextSibling());
}
bool Node::IsElementNode() const {
  return GetNode()->IsElementNode();
}
bool Node::IsTextNode() const {
  return GetNode()->IsTextNode();
}

}  // namespace webagents
