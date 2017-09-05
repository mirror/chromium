// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_option_element.h"

#include "core/html/HTMLOptionElement.h"
#include "public/platform/WebString.h"

namespace webagents {

HTMLOptionElement::HTMLOptionElement(blink::HTMLOptionElement& element)
    : HTMLElement(element) {}

blink::HTMLOptionElement& HTMLOptionElement::GetHTMLOptionElement() const {
  return toHTMLOptionElement(GetNode());
}


base::string16 HTMLOptionElement::label() const {
  return blink::WebString(GetHTMLOptionElement().label()).Utf16();
}
base::string16 HTMLOptionElement::value() const {
  return blink::WebString(GetHTMLOptionElement().value()).Utf16();
}

}  // namespace webagents
