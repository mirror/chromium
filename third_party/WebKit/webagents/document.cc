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

blink::Document* Document::GetDocument() const {
  return blink::ToDocument(GetNode());
}

const blink::WebURL Document::URL() {
  return GetDocument()->Url();
}
std::unique_ptr<Element> Document::documentElement() const {
  return std::make_unique<Element>(GetDocument()->documentElement());
}
std::unique_ptr<HTMLElement> Document::body() const {
  return std::make_unique<HTMLElement>(GetDocument()->body());
}
std::unique_ptr<const HTMLHeadElement> Document::head() const {
  return std::make_unique<HTMLHeadElement>(GetDocument()->head());
}

void Document::UpdateStyleAndLayoutTree() {
  GetDocument()->UpdateStyleAndLayoutTree();
}

}  // namespace webagents
