// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_META_ELEMENT_H_
#define WEBAGENTS_HTML_META_ELEMENT_H_

#include "third_party/WebKit/webagents/html_element.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLMetaElement;
class WebString;
}  // namespace blink

namespace webagents {

// https://html.spec.whatwg.org/#the-meta-element
class WEBAGENTS_EXPORT HTMLMetaElement final : public HTMLElement {
 public:
  // [CEReactions] attribute DOMString name;
  blink::WebString name() const;
  // [CEReactions] attribute DOMString content;
  blink::WebString content() const;

#if INSIDE_BLINK
 private:
  friend class EventTarget;
  explicit HTMLMetaElement(blink::HTMLMetaElement&);
#endif
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_META_ELEMENT_H_
