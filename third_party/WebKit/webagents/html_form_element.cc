// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_form_element.h"

#include "core/html/HTMLFormElement.h"
#include "public/platform/WebString.h"
#include "third_party/WebKit/webagents/html_form_controls_collection.h"

namespace webagents {

HTMLFormElement::HTMLFormElement(blink::HTMLFormElement& element)
    : HTMLElement(element) {}

HTMLFormControlsCollection HTMLFormElement::elements() const {
  // elements is logically const, requires const_cast.
  return HTMLFormControlsCollection(*const_cast<blink::HTMLFormElement&>(
                                         ConstUnwrap<blink::HTMLFormElement>())
                                         .elements());
}

blink::WebString HTMLFormElement::action() const {
  return ConstUnwrap<blink::HTMLFormElement>().Action();
}
blink::WebString HTMLFormElement::autocomplete() const {
  return ConstUnwrap<blink::HTMLFormElement>().FastGetAttribute(
      blink::HTMLNames::autocompleteAttr);
}
blink::WebString HTMLFormElement::name() const {
  return ConstUnwrap<blink::HTMLFormElement>().GetName();
}

}  // namespace webagents
