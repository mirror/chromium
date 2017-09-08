// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_SELECT_ELEMENT_H_
#define WEBAGENTS_HTML_SELECT_ELEMENT_H_

#include "base/optional.h"
#include "third_party/WebKit/webagents/html_element.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLSelectElement;
class WebString;
}  // namespace blink

namespace webagents {

class HTMLFormElement;
class HTMLOptionsCollection;

// https://html.spec.whatwg.org/#the-select-element
class WEBAGENTS_EXPORT HTMLSelectElement final : public HTMLElement {
 public:
  // [CEReactions] attribute boolean disabled;
  bool disabled() const;
  // readonly attribute HTMLFormElement? form;
  base::Optional<HTMLFormElement> form() const;
  // [CEReactions] attribute DOMString name;
  blink::WebString name() const;
  // readonly attribute DOMString type;
  blink::WebString type() const;
  // [SameObject] readonly attribute HTMLOptionsCollection options;
  HTMLOptionsCollection options() const;
  // attribute DOMString value;
  blink::WebString value() const;

  // Not standard.
  blink::WebString NotStandardSuggestedValue() const;
#if INSIDE_BLINK
 private:
  friend class EventTarget;
  explicit HTMLSelectElement(blink::HTMLSelectElement&);
#endif
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_SELECT_ELEMENT_H_
