// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_option_element.h"

#include "core/html/HTMLOptionElement.h"
#include "public/platform/WebString.h"

namespace webagents {

HTMLOptionElement::HTMLOptionElement(blink::HTMLOptionElement& element)
    : HTMLElement(element) {}

blink::WebString HTMLOptionElement::label() const {
  return ConstUnwrap<blink::HTMLOptionElement>().label();
}
blink::WebString HTMLOptionElement::value() const {
  return ConstUnwrap<blink::HTMLOptionElement>().value();
}
blink::WebString HTMLOptionElement::text() const {
  return ConstUnwrap<blink::HTMLOptionElement>().text();
}

}  // namespace webagents
