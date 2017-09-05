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
}

namespace webagents {

class HTMLOptionElement;

// https://html.spec.whatwg.org/#the-input-element
class WEBAGENTS_EXPORT HTMLInputElement : public HTMLElement {
 public:
  // [CEReactions] attribute boolean disabled;
  bool disabled() const;
  // readonly attribute HTMLFormElement? form;
  base::Optional<HTMLFormElement> form() const;
  // [CEReactions] attribute boolean readOnly;
  bool readOnly() const;
  // [CEReactions] attribute DOMString type;
  std::string type() const;

  // Not standard.
  std::string NotStandardEditingValue() const;
  // This could be moved out of blink.
  std::vector<HTMLOptionElement> NotStandardFilteredDataListOptions() const;
  int NotStandardSelectionStart() const;
  int NotStandardSelectionEnd() const;
  std::string NotStandardSuggestedValue() const;
 private:
  friend class EventTarget;
  explicit HTMLInputElement(blink::HTMLInputElement&);
  blink::HTMLInputElement& GetHTMLInputElement() const;
};

}  // namespace webagents

#endif  // WEBAGENTS_HTML_INPUT_ELEMENT_H_
