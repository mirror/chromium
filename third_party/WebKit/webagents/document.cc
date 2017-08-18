// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/document.h"

#include "third_party/WebKit/Source/core/dom/Document.h"
#include "third_party/WebKit/webagents/element.h"
#include "third_party/WebKit/webagents/html_element.h"
#include "third_party/WebKit/webagents/html_head_element.h"

namespace webagents {

Document::Document(blink::Document* document) : Node(document) {}

Document* Document::Create(blink::Document* document) {
  return document ? new Document(document) : nullptr;
}

blink::Document* Document::GetDocument() const {
  return ToDocument(GetNode());
}

const blink::WebURL Document::URL() {
  return GetDocument()->Url();
}
Element* Document::documentElement() const {
  return Element::Create(GetDocument()->documentElement());
}
HTMLElement* Document::body() const {
  return HTMLElement::Create(GetDocument()->body());
}
const HTMLHeadElement* Document::head() const {
  return HTMLHeadElement::Create(GetDocument()->head());
}

}  // namespace webagents
