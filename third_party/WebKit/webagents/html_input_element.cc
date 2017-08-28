// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_input_element.h"

#include "third_party/WebKit/Source/core/html/HTMLInputElement.h"

namespace webagents {

HTMLInputElement::HTMLInputElement(Node& node)
    : HTMLElement(toHTMLInputElement(node.GetNode())) {}

HTMLInputElement::HTMLInputElement(blink::HTMLInputElement* element)
    : HTMLElement(element) {}

blink::HTMLInputElement* HTMLInputElement::GetHTMLInputElement() const {
  return toHTMLInputElement(GetNode());
}

std::string HTMLInputElement::type() const {
  return blink::WebString(GetHTMLInputElement()->type()).Ascii();
}

}  // namespace webagents
