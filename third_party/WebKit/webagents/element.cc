// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/element.h"

#include "core/dom/Element.h"
#include "core/dom/NodeComputedStyle.h"
#include "core/html/HTMLFormControlElement.h"
#include "core/style/ComputedStyle.h"
#include "public/platform/WebString.h"

namespace webagents {

Element::Element(blink::Element& element) : Node(element) {}

blink::WebString Element::tagName() const {
  return ConstUnwrap<blink::Element>().tagName();
}
blink::WebString Element::getAttribute(blink::WebString qualifiedName) const {
  return ConstUnwrap<blink::Element>().getAttribute(qualifiedName);
}
blink::WebString Element::GetIdAttribute() const {
  return ConstUnwrap<blink::Element>().GetIdAttribute();
}
blink::WebString Element::GetClassAttribute() const {
  return ConstUnwrap<blink::Element>().GetClassAttribute();
}
void Element::setInnerHTML(blink::WebString innerHTML) {
  Unwrap<blink::Element>().setInnerHTML(innerHTML);
}

blink::WebRect Element::NotStandardBoundsInViewport() const {
  return ConstUnwrap<blink::Element>().BoundsInViewport();
}
bool Element::NotStandardIsVisible() const {
  const blink::ComputedStyle* style = ConstUnwrap<blink::Element>().GetComputedStyle();
  if (!style)
    return false;
  return (style->Display() != blink::EDisplay::kNone &&
          style->Visibility() != blink::EVisibility::kHidden &&
          style->Opacity() != 0);
}
bool Element::NotStandardIsAutofilled() const {
  if (blink::IsHTMLFormControlElement(ConstUnwrap<blink::Element>()))
    return blink::ToHTMLFormControlElement(ConstUnwrap<blink::Element>()).IsAutofilled();
  return false;
}

}  // namespace webagents
