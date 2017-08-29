// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/node.h"

#include "core/dom/ContainerNode.h"
#include "core/dom/Node.h"

namespace webagents {

Node::Node(blink::Node* node) : EventTarget(node) {}

blink::Node* Node::GetNode() const {
  return GetEventTarget()->ToNode();
}

Node Node::parentNode() const {
  return Node(GetNode()->parentNode());
}
Node Node::firstChild() const {
  return Node(GetNode()->firstChild());
}
Node Node::nextSibling() const {
  return Node(GetNode()->nextSibling());
}
bool Node::IsElementNode() const {
  return GetNode()->IsElementNode();
}
bool Node::IsTextNode() const {
  return GetNode()->IsTextNode();
}

}  // namespace webagents
