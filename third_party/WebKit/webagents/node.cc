// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/node.h"

#include "core/dom/ContainerNode.h"
#include "core/dom/Document.h"
#include "core/dom/Node.h"
#include "third_party/WebKit/webagents/document.h"

namespace webagents {

Node::Node(blink::Node& node) : EventTarget(node) {}

blink::Node& Node::GetNode() const {
  return *GetEventTarget().ToNode();
}

Document Node::ownerDocument() const {
  blink::Document* doc = GetNode().ownerDocument();
  DCHECK(doc);
  return Document(*doc);
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

bool Node::NotStandardIsFocused() const {
  return GetNode().IsFocused();
}

}  // namespace webagents
