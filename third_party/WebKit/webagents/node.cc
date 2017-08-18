// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/node.h"

#include "third_party/WebKit/Source/core/dom/Node.h"

namespace webagents {

Node::Node(blink::Node* node) : EventTarget(node) {}

Node* Node::Create(blink::Node* node) {
  return node ? new Node(node) : nullptr;
}

blink::Node* Node::GetNode() const {
  return GetEventTarget()->ToNode();
}

bool Node::IsElementNode() const {
  return GetNode()->IsElementNode();
}
bool Node::IsTextNode() const {
  return GetNode()->IsTextNode();
}

}  // namespace webagents
