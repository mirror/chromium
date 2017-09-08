// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_FORM_ELEMENT_H_
#define WEBAGENTS_HTML_FORM_ELEMENT_H_

#include "third_party/WebKit/webagents/html_element.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLFormElement;
class WebString;
}  // namespace blink

namespace webagents {

class HTMLFormControlsCollection;

// https://html.spec.whatwg.org/#the-form-element
class WEBAGENTS_EXPORT HTMLFormElement final : public HTMLElement {
 public:
  // [CEReactions] attribute USVString action;
  blink::WebString action() const;
  // [CEReactions] attribute DOMString autocomplete;
  blink::WebString autocomplete() const;
  // [CEReactions] attribute DOMString name;
  blink::WebString name() const;
  // [SameObject] readonly attribute HTMLFormControlsCollection elements;
  HTMLFormControlsCollection elements() const;

#if INSIDE_BLINK
 private:
  friend class EventTarget;
  explicit HTMLFormElement(blink::HTMLFormElement&);
#endif
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_FORM_ELEMENT_H_
