// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/element.h"

#include "third_party/WebKit/Source/core/dom/Element.h"
#include "third_party/WebKit/Source/core/dom/NodeComputedStyle.h"
#include "third_party/WebKit/Source/core/style/ComputedStyle.h"
#include "third_party/WebKit/public/platform/WebString.h"

namespace webagents {

Element::Element(Node& node) : Node(blink::ToElement(node.GetNode())) {}

Element::Element(blink::Element* element) : Node(element) {}

blink::Element* Element::GetElement() const {
  return blink::ToElement(GetNode());
}

std::string Element::tagName() const {
  return blink::WebString(GetElement()->tagName()).Ascii();
}
std::string Element::getAttribute(std::string qualifiedName) const {
  return blink::WebString(GetElement()->getAttribute(
                              // TODO: FromUTF8?
                              blink::WebString::FromASCII(qualifiedName)))
      .Ascii();
}
std::string Element::GetIdAttribute() const {
  return blink::WebString(GetElement()->GetIdAttribute()).Ascii();
}
std::string Element::GetClassAttribute() const {
  return blink::WebString(GetElement()->GetClassAttribute()).Ascii();
}
void Element::setInnerHTML(std::string innerHTML) {
  GetElement()->setInnerHTML(blink::WebString::FromASCII(innerHTML));
}

bool Element::IsVisible() const {
  const blink::ComputedStyle* style = GetElement()->GetComputedStyle();
  if (!style)
    return false;
  return (style->Display() != blink::EDisplay::kNone &&
          style->Visibility() != blink::EVisibility::kHidden &&
          style->Opacity() != 0);
}

}  // namespace webagents
