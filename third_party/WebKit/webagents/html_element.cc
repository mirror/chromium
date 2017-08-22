// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_element.h"

#include "third_party/WebKit/Source/core/html/HTMLElement.h"

namespace webagents {

HTMLElement::HTMLElement(Node& node)
    : Element(blink::ToHTMLElement(node.GetNode())) {}

HTMLElement::HTMLElement(blink::HTMLElement* element) : Element(element) {}

blink::HTMLElement* HTMLElement::GetHTMLElement() const {
  return blink::ToHTMLElement(GetNode());
}

}  // namespace webagents
