// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_OPTION_ELEMENT_H_
#define WEBAGENTS_HTML_OPTION_ELEMENT_H_

#include "third_party/WebKit/webagents/html_element.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLOptionElement;
class WebString;
}  // namespace blink

namespace webagents {

// https://html.spec.whatwg.org/#the-option-element
class WEBAGENTS_EXPORT HTMLOptionElement final : public HTMLElement {
 public:
  // [CEReactions] attribute DOMString label;
  blink::WebString label() const;
  // [CEReactions] attribute DOMString value;
  blink::WebString value() const;
  // [CEReactions] attribute DOMString text;
  blink::WebString text() const;

#if INSIDE_BLINK
 private:
  friend class EventTarget;
  friend class HTMLInputElement;
  explicit HTMLOptionElement(blink::HTMLOptionElement&);
#endif
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_OPTION_ELEMENT_H_
