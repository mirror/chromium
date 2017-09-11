// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBAGENTS_HTML_INPUT_ELEMENT_H_
#define WEBAGENTS_HTML_INPUT_ELEMENT_H_

#include <string>
#include <vector>
#include "base/optional.h"
#include "third_party/WebKit/webagents/html_element.h"
#include "third_party/WebKit/webagents/webagents_export.h"

namespace blink {
class HTMLInputElement;
class WebString;
}  // namespace blink

namespace webagents {

class HTMLOptionElement;

// https://html.spec.whatwg.org/#the-input-element
class WEBAGENTS_EXPORT HTMLInputElement final : public HTMLElement {
 public:
  // [CEReactions] attribute DOMString autocomplete;
  blink::WebString autocomplete() const;
  // attribute boolean checked;
  bool checked() const;
  // [CEReactions] attribute boolean disabled;
  bool disabled() const;
  // readonly attribute HTMLFormElement? form;
  base::Optional<HTMLFormElement> form() const;
  //  [CEReactions] attribute long maxLength;
  int maxLength() const;
  // [CEReactions] attribute DOMString name;
  blink::WebString name() const;
  // [CEReactions] attribute boolean readOnly;
  bool readOnly() const;
  // [CEReactions] attribute DOMString type;
  blink::WebString type() const;
  // [CEReactions] attribute [TreatNullAs=EmptyString] DOMString value;
  blink::WebString value() const;

  // Not standard.
  blink::WebString NotStandardEditingValue() const;
  // This could be moved out of blink.
  std::vector<HTMLOptionElement> NotStandardFilteredDataListOptions() const;
  blink::WebString NotStandardNameForAutofill() const;
  int NotStandardSelectionStart() const;
  int NotStandardSelectionEnd() const;
  blink::WebString NotStandardSuggestedValue() const;

#if INSIDE_BLINK
 private:
  friend class EventTarget;
  explicit HTMLInputElement(blink::HTMLInputElement&);
#endif
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_INPUT_ELEMENT_H_
