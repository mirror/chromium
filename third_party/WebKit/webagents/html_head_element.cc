// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/html_head_element.h"

#include "third_party/WebKit/Source/core/html/HTMLHeadElement.h"

namespace webagents {

HTMLHeadElement::HTMLHeadElement(Node& node)
    : HTMLElement(toHTMLHeadElement(node.GetNode())) {}

HTMLHeadElement::HTMLHeadElement(blink::HTMLHeadElement* element)
    : HTMLElement(element) {}

blink::HTMLHeadElement* HTMLHeadElement::GetHTMLHeadElement() const {
  return toHTMLHeadElement(GetNode());
}

}  // namespace webagents
