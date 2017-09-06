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

bool HTMLInputElement::disabled() const {
  return ConstUnwrap<blink::HTMLInputElement>().IsDisabledFormControl();
}
base::Optional<HTMLFormElement> HTMLInputElement::form() const {
  return Create<HTMLFormElement, blink::HTMLFormElement>(
      ConstUnwrap<blink::HTMLInputElement>().Form());
}
bool HTMLInputElement::readOnly() const {
  return ConstUnwrap<blink::HTMLInputElement>().IsReadOnly();
}
blink::WebString HTMLInputElement::type() const {
  return ConstUnwrap<blink::HTMLInputElement>().type();
}

blink::WebString HTMLInputElement::NotStandardEditingValue() const {
  return ConstUnwrap<blink::HTMLInputElement>().InnerEditorValue();
}
std::vector<HTMLOptionElement>
HTMLInputElement::NotStandardFilteredDataListOptions() const {
  std::vector<HTMLOptionElement> result;
  for (auto & option : ConstUnwrap<blink::HTMLInputElement>().FilteredDataListOptions())
    result.push_back(HTMLOptionElement(*option));
  return result;
}
int HTMLInputElement::NotStandardSelectionStart() const {
  return ConstUnwrap<blink::HTMLInputElement>().selectionStart();
}
int HTMLInputElement::NotStandardSelectionEnd() const {
  return ConstUnwrap<blink::HTMLInputElement>().selectionEnd();
}
blink::WebString HTMLInputElement::NotStandardSuggestedValue() const {
  return ConstUnwrap<blink::HTMLInputElement>().SuggestedValue();
}

}  // namespace webagents
