// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/node.h"

#include "core/HTMLElementTypeHelpers.h"
#include "core/dom/ContainerNode.h"
#include "core/dom/Node.h"
#include "core/dom/Text.h"
#include "core/html/HTMLHeadElement.h"
#include "core/html/HTMLInputElement.h"
#include "core/html/HTMLMetaElement.h"
#include "third_party/WebKit/webagents/html_head_element.h"
#include "third_party/WebKit/webagents/html_input_element.h"
#include "third_party/WebKit/webagents/html_meta_element.h"
#include "third_party/WebKit/webagents/text.h"

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

Element Node::ToElement() const {
  return Element(blink::ToElement(GetNode()));
}
HTMLHeadElement Node::ToHTMLHeadElement() const {
  return HTMLHeadElement(toHTMLHeadElement(GetNode()));
}
HTMLInputElement Node::ToHTMLInputElement() const {
  return HTMLInputElement(toHTMLInputElement(GetNode()));
}
HTMLMetaElement Node::ToHTMLMetaElement() const {
  return HTMLMetaElement(toHTMLMetaElement(GetNode()));
}
Text Node::ToText() const {
  return Text(blink::ToText(GetNode()));
}

}  // namespace webagents
