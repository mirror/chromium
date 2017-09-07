// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_select_element.h"

#include "core/html/HTMLSelectElement.h"
#include "third_party/WebKit/webagents/html_form_element.h"
#include "third_party/WebKit/webagents/html_options_collection.h"

namespace webagents {

HTMLSelectElement::HTMLSelectElement(blink::HTMLSelectElement& element)
    : HTMLElement(element) {}

base::Optional<HTMLFormElement> HTMLSelectElement::form() const {
  return Create<HTMLFormElement, blink::HTMLFormElement>(
      ConstUnwrap<blink::HTMLSelectElement>().Form());
}
HTMLOptionsCollection HTMLSelectElement::options() const {
  // options is logically const, requires const_cast.
  return HTMLOptionsCollection(*const_cast<blink::HTMLSelectElement&>(
                                    ConstUnwrap<blink::HTMLSelectElement>())
                                    .options());
}
blink::WebString HTMLSelectElement::value() const {
  return ConstUnwrap<blink::HTMLSelectElement>().value();
}

}  // namespace webagents
