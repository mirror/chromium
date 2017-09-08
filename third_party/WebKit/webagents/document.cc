// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/document.h"

#include "core/dom/Document.h"
#include "public/platform/WebString.h"
#include "public/platform/WebURL.h"
#include "third_party/WebKit/webagents/element.h"
#include "third_party/WebKit/webagents/html_all_collection.h"
#include "third_party/WebKit/webagents/html_element.h"
#include "third_party/WebKit/webagents/html_head_element.h"

namespace webagents {

Document::Document(blink::Document& document) : Node(document) {}

HTMLAllCollection Document::all() const {
  // all is logically const, requires const_cast.
  return HTMLAllCollection(
      *const_cast<blink::Document&>(ConstUnwrap<blink::Document>()).all());
}
blink::WebURL Document::URL() const {
  return ConstUnwrap<blink::Document>().Url();
}
base::Optional<Element> Document::documentElement() const {
  return Create<Element, blink::Element>(
      ConstUnwrap<blink::Document>().documentElement());
}
blink::WebString Document::title() const {
  return ConstUnwrap<blink::Document>().title();
}
base::Optional<HTMLElement> Document::body() const {
  return Create<HTMLElement, blink::HTMLElement>(
      ConstUnwrap<blink::Document>().body());
}
base::Optional<HTMLHeadElement> Document::head() const {
  return Create<HTMLHeadElement, blink::HTMLHeadElement>(
      ConstUnwrap<blink::Document>().head());
}
base::Optional<Element> Document::activeElement() const {
  return Create<Element, blink::Element>(
      ConstUnwrap<blink::Document>().ActiveElement());
}

bool Document::isSecureContext() const {
  return ConstUnwrap<blink::Document>().IsSecureContext();
}
blink::WebURL Document::CompleteURL(blink::WebString url) const {
  return ConstUnwrap<blink::Document>().CompleteURL(url);
}

bool Document::NotStandardIsAttachedToFrame() const {
  return !!ConstUnwrap<blink::Document>().GetFrame();
}
void Document::NotStandardUpdateStyleAndLayoutTree() {
  Unwrap<blink::Document>().UpdateStyleAndLayoutTree();
}

}  // namespace webagents
