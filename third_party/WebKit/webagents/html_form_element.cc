// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_form_element.h"

#include "core/html/HTMLFormElement.h"

namespace webagents {

HTMLFormElement::HTMLFormElement(blink::HTMLFormElement& element)
    : HTMLElement(element) {}

blink::HTMLFormElement& HTMLFormElement::GetHTMLFormElement() const {
  return toHTMLFormElement(GetNode());
}

}  // namespace webagents
