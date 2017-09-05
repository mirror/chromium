// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/document.h"

#include "core/dom/Document.h"
#include "third_party/WebKit/webagents/element.h"
#include "third_party/WebKit/webagents/html_all_collection.h"
#include "third_party/WebKit/webagents/html_element.h"
#include "third_party/WebKit/webagents/html_head_element.h"

namespace webagents {

Document::Document(blink::Document& document) : Node(document) {}

blink::Document& Document::GetDocument() const {
  return blink::ToDocument(GetNode());
}

HTMLAllCollection Document::all() const {
  return HTMLAllCollection(*GetDocument().all());
}
const blink::WebURL Document::URL() const {
  return GetDocument().Url();
}
base::Optional<Element> Document::documentElement() const {
  return Create<Element, blink::Element>(GetDocument().documentElement());
}
base::Optional<HTMLElement> Document::body() const {
  return Create<HTMLElement, blink::HTMLElement>(GetDocument().body());
}
base::Optional<HTMLHeadElement> Document::head() const {
  return Create<HTMLHeadElement, blink::HTMLHeadElement>(GetDocument().head());
}
base::Optional<Element> Document::activeElement() const {
  return Create<Element, blink::Element>(GetDocument().ActiveElement());
}

bool Document::isSecureContext() const {
  return GetDocument().IsSecureContext();
}

void Document::NotStandardUpdateStyleAndLayoutTree() const {
  GetDocument().UpdateStyleAndLayoutTree();
}

}  // namespace webagents
