// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/autofill_element.h"

#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/webagents/element.h"
#include "third_party/WebKit/webagents/html_form_element.h"
#include "third_party/WebKit/webagents/html_input_element.h"
#include "third_party/WebKit/webagents/html_select_element.h"
#include "third_party/WebKit/webagents/html_text_area_element.h"

namespace autofill {

AutofillElement::~AutofillElement() {}

AutofillElement::AutofillElement(
    webagents::Element element,
    bool disabled,
    base::Optional<webagents::HTMLFormElement> form,
    blink::WebString name,
    bool readOnly,
    blink::WebString type,
    blink::WebString value,
    blink::WebString suggested_value,
    blink::WebString editing_value,
    int selection_start,
    int selection_end)
    : element_(element),
      disabled_(disabled),
      form_(form),
      name_(name),
      readOnly_(readOnly),
      type_(type),
      value_(value),
      suggested_value_(suggested_value),
      editing_value_(editing_value),
      selection_start_(selection_start),
      selection_end_(selection_end) {}

AutofillElement::AutofillElement(const AutofillElement& autofill_element)
    : element_(autofill_element.element_),
      disabled_(autofill_element.disabled_),
      form_(autofill_element.form_),
      name_(autofill_element.name_),
      readOnly_(autofill_element.readOnly_),
      type_(autofill_element.type_),
      value_(autofill_element.value_),
      suggested_value_(autofill_element.suggested_value_),
      editing_value_(autofill_element.editing_value_),
      selection_start_(autofill_element.selection_start_),
      selection_end_(autofill_element.selection_end_) {}

AutofillElement::AutofillElement(webagents::HTMLInputElement element)
    : AutofillElement(element,
                      element.disabled(),
                      element.form(),
                      element.name(),
                      element.readOnly(),
                      element.type(),
                      element.value(),
                      element.NotStandardSuggestedValue(),
                      element.NotStandardEditingValue(),
                      element.NotStandardSelectionStart(),
                      element.NotStandardSelectionEnd()) {}

AutofillElement::AutofillElement(webagents::HTMLTextAreaElement element)
    : AutofillElement(element,
                      element.disabled(),
                      element.form(),
                      element.name(),
                      element.readOnly(),
                      element.type(),
                      element.value(),
                      element.NotStandardSuggestedValue(),
                      element.NotStandardEditingValue(),
                      element.NotStandardSelectionStart(),
                      element.NotStandardSelectionEnd()) {}

AutofillElement::AutofillElement(webagents::HTMLSelectElement element)
    : AutofillElement(element,
                      element.disabled(),
                      element.form(),
                      element.name(),
                      false /* readOnly */,
                      element.type(),
                      element.value(),
                      element.NotStandardSuggestedValue(),
                      "" /* editing_value */,
                      0 /* selection_start */,
                      0 /* selection_end */) {}

base::Optional<AutofillElement> AutofillElement::FromElement(
    webagents::Element element) {
  // TODO: complete this.
  if (element.IsHTMLInputElement())
    return AutofillElement(element.ToHTMLInputElement());
  if (element.IsHTMLTextAreaElement())
    return AutofillElement(element.ToHTMLTextAreaElement());
  if (element.IsHTMLSelectElement())
    return AutofillElement(element.ToHTMLSelectElement());

  return base::Optional<AutofillElement>();
}

}  // namespace autofill
