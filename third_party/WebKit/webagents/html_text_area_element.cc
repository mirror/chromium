// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_text_area_element.h"

#include "core/html/HTMLTextAreaElement.h"

namespace webagents {

HTMLTextAreaElement::HTMLTextAreaElement(blink::HTMLTextAreaElement& element)
    : HTMLElement(element) {}

blink::HTMLTextAreaElement& HTMLTextAreaElement::GetHTMLTextAreaElement() const {
  return toHTMLTextAreaElement(GetNode());
}

bool HTMLTextAreaElement::disabled() const {
  return GetHTMLTextAreaElement().IsDisabledFormControl();
}
bool HTMLTextAreaElement::readOnly() const {
  return GetHTMLTextAreaElement().IsReadOnly();
}

std::string HTMLTextAreaElement::NotStandardEditingValue() const {
  return blink::WebString(GetHTMLTextAreaElement().InnerEditorValue()).Ascii();
}
int HTMLTextAreaElement::NotStandardSelectionStart() const {
  return GetHTMLTextAreaElement().selectionStart();
}
int HTMLTextAreaElement::NotStandardSelectionEnd() const {
  return GetHTMLTextAreaElement().selectionEnd();
}
std::string HTMLTextAreaElement::NotStandardSuggestedValue() const {
  return blink::WebString(GetHTMLTextAreaElement().SuggestedValue()).Ascii();
}

}  // namespace webagents
