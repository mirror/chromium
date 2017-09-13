// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_TEXT_AREA_ELEMENT_H_
#define WEBAGENTS_HTML_TEXT_AREA_ELEMENT_H_

#include "base/optional.h"
#include "third_party/WebKit/webagents/html_element.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLTextAreaElement;
class WebString;
}  // namespace blink

namespace webagents {

class HTMLFormElement;

// https://html.spec.whatwg.org/#the-textarea-element
class WEBAGENTS_EXPORT HTMLTextAreaElement final : public HTMLElement {
 public:
  // [CEReactions] attribute boolean disabled;
  bool disabled() const;
  // readonly attribute HTMLFormElement? form;
  base::Optional<HTMLFormElement> form() const;
  // [CEReactions] attribute DOMString name;
  blink::WebString name() const;
  // [CEReactions] attribute boolean readOnly;
  bool readOnly() const;
  // readonly attribute DOMString type;
  blink::WebString type() const;
  // [CEReactions] attribute [TreatNullAs=EmptyString] DOMString value;
  blink::WebString value() const;

  // Not standard.
  blink::WebString NotStandardEditingValue() const;
  int NotStandardSelectionStart() const;
  int NotStandardSelectionEnd() const;
  blink::WebString NotStandardSuggestedValue() const;

#if INSIDE_BLINK
 private:
  friend class EventTarget;
  explicit HTMLTextAreaElement(blink::HTMLTextAreaElement&);
#endif
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_TEXT_AREA_ELEMENT_H_
