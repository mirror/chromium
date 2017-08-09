// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/web_agent/api/node.h"

#include "core/dom/Node.h"

namespace web {

Node::Node(blink::Node* node) : EventTarget(node) {}

Node* Node::Create(blink::Node* node) {
  return node ? new Node(node) : nullptr;
}

blink::Node* Node::GetNode() const {
  return GetEventTarget()->ToNode();
}

}  // namespace web
