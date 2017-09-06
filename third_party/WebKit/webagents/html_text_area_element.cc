// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_text_area_element.h"

#include "core/html/HTMLTextAreaElement.h"
#include "public/platform/WebString.h"

namespace webagents {

HTMLTextAreaElement::HTMLTextAreaElement(blink::HTMLTextAreaElement& element)
    : HTMLElement(element) {}

bool HTMLTextAreaElement::disabled() const {
  return ConstUnwrap<blink::HTMLTextAreaElement>().IsDisabledFormControl();
}
bool HTMLTextAreaElement::readOnly() const {
  return ConstUnwrap<blink::HTMLTextAreaElement>().IsReadOnly();
}

blink::WebString HTMLTextAreaElement::NotStandardEditingValue() const {
  return ConstUnwrap<blink::HTMLTextAreaElement>().InnerEditorValue();
}
int HTMLTextAreaElement::NotStandardSelectionStart() const {
  return ConstUnwrap<blink::HTMLTextAreaElement>().selectionStart();
}
int HTMLTextAreaElement::NotStandardSelectionEnd() const {
  return ConstUnwrap<blink::HTMLTextAreaElement>().selectionEnd();
}
blink::WebString HTMLTextAreaElement::NotStandardSuggestedValue() const {
  return ConstUnwrap<blink::HTMLTextAreaElement>().SuggestedValue();
}

}  // namespace webagents
