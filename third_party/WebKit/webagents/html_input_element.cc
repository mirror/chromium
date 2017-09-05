// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_input_element.h"

#include "core/html/HTMLFormElement.h"
#include "core/html/HTMLInputElement.h"
#include "public/platform/WebString.h"
#include "third_party/WebKit/webagents/html_form_element.h"
#include "third_party/WebKit/webagents/html_option_element.h"

namespace webagents {

HTMLInputElement::HTMLInputElement(blink::HTMLInputElement& element)
    : HTMLElement(element) {}

blink::HTMLInputElement& HTMLInputElement::GetHTMLInputElement() const {
  return toHTMLInputElement(GetNode());
}

bool HTMLInputElement::disabled() const {
  return GetHTMLInputElement().IsDisabledFormControl();
}
base::Optional<HTMLFormElement> HTMLInputElement::form() const {
  return Create<HTMLFormElement, blink::HTMLFormElement>(
      GetHTMLInputElement().Form());
}
bool HTMLInputElement::readOnly() const {
  return GetHTMLInputElement().IsReadOnly();
}
std::string HTMLInputElement::type() const {
  return blink::WebString(GetHTMLInputElement().type()).Ascii();
}

std::string HTMLInputElement::NotStandardEditingValue() const {
  return blink::WebString(GetHTMLInputElement().InnerEditorValue()).Ascii();
}
std::vector<HTMLOptionElement>
HTMLInputElement::NotStandardFilteredDataListOptions() const {
  std::vector<HTMLOptionElement> result;
  for (auto & option : GetHTMLInputElement().FilteredDataListOptions())
    result.push_back(HTMLOptionElement(*option));
  return result;
}
int HTMLInputElement::NotStandardSelectionStart() const {
  return GetHTMLInputElement().selectionStart();
}
int HTMLInputElement::NotStandardSelectionEnd() const {
  return GetHTMLInputElement().selectionEnd();
}
std::string HTMLInputElement::NotStandardSuggestedValue() const {
  return blink::WebString(GetHTMLInputElement().SuggestedValue()).Ascii();
}

}  // namespace webagents
