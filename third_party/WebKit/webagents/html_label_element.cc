// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_label_element.h"

#include "core/html/HTMLLabelElement.h"
#include "core/html/LabelableElement.h"

namespace webagents {

HTMLLabelElement::HTMLLabelElement(blink::HTMLLabelElement& element)
    : HTMLElement(element) {}

base::Optional<HTMLElement> HTMLLabelElement::control() const {
  return Create<HTMLElement, blink::HTMLElement>(
      ConstUnwrap<blink::HTMLLabelElement>().control());
}

}  // namespace webagents
