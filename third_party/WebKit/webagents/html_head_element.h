// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_HEAD_ELEMENT_H_
#define WEBAGENTS_HTML_HEAD_ELEMENT_H_

#include <string>
#include "third_party/WebKit/webagents/html_element.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLHeadElement;
class WebString;
}  // namespace blink

namespace webagents {

// https://html.spec.whatwg.org/#htmlheadelement
class WEBAGENTS_EXPORT HTMLHeadElement final : public HTMLElement {
 public:
  // [CEReactions] attribute DOMString type;
  blink::WebString type() const;

#if INSIDE_BLINK
 private:
  friend class EventTarget;
  explicit HTMLHeadElement(blink::HTMLHeadElement&);
#endif
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_HEAD_ELEMENT_H_
