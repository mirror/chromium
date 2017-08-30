// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/document.h"

#include "core/dom/Document.h"
#include "third_party/WebKit/webagents/element.h"
#include "third_party/WebKit/webagents/html_element.h"
#include "third_party/WebKit/webagents/html_head_element.h"

namespace webagents {

Document::Document(blink::Document* document) : Node(document) {}

blink::Document* Document::GetDocument() const {
  return blink::ToDocument(GetNode());
}

const blink::WebURL Document::URL() const {
  return GetDocument()->Url();
}
Element Document::documentElement() const {
  return Element(GetDocument()->documentElement());
}
HTMLElement Document::body() const {
  return HTMLElement(GetDocument()->body());
}
HTMLHeadElement Document::head() const {
  return HTMLHeadElement(GetDocument()->head());
}

void Document::UpdateStyleAndLayoutTree() const {
  GetDocument()->UpdateStyleAndLayoutTree();
}

}  // namespace webagents
