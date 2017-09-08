// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/element.h"

#include "core/dom/Element.h"
#include "core/dom/NodeComputedStyle.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/html/HTMLFormControlElement.h"
#include "core/style/ComputedStyle.h"
#include "public/platform/WebString.h"
#include "third_party/WebKit/webagents/css_style_declaration.h"
#include "third_party/WebKit/webagents/html_collection.h"

namespace webagents {

Element::Element(blink::Element& element) : Node(element) {}

blink::WebString Element::tagName() const {
  return ConstUnwrap<blink::Element>().tagName();
}
blink::WebString Element::getAttribute(
    const blink::WebString qualifiedName) const {
  return ConstUnwrap<blink::Element>().getAttribute(qualifiedName);
}
bool Element::hasAttribute(const blink::WebString qualifiedName) const {
  return ConstUnwrap<blink::Element>().hasAttribute(qualifiedName);
}
HTMLCollection Element::getElementsByTagName(
    const blink::WebString qualifiedName) const {
  // getElementsByTagName is logically const, requires const_cast.
  return HTMLCollection(
      *const_cast<blink::Element&>(ConstUnwrap<blink::Element>())
           .getElementsByTagName(qualifiedName));
}
void Element::setInnerHTML(const blink::WebString innerHTML) {
  Unwrap<blink::Element>().setInnerHTML(innerHTML);
}

CSSStyleDeclaration Element::getComputedStyle() const {
  const blink::Element* element = &ConstUnwrap<blink::Element>();
  return CSSStyleDeclaration(
      *element->GetDocument().domWindow()->getComputedStyle(
          const_cast<blink::Element*>(element), blink::WebString()));
}
blink::WebString Element::GetIdAttribute() const {
  return ConstUnwrap<blink::Element>().GetIdAttribute();
}
blink::WebString Element::GetClassAttribute() const {
  return ConstUnwrap<blink::Element>().GetClassAttribute();
}
bool Element::IsFocusable() const {
  return ConstUnwrap<blink::Element>().IsFocusable();
}

blink::WebRect Element::NotStandardBoundsInViewport() const {
  return ConstUnwrap<blink::Element>().BoundsInViewport();
}
bool Element::NotStandardIsVisible() const {
  const blink::ComputedStyle* style =
      ConstUnwrap<blink::Element>().GetComputedStyle();
  if (!style)
    return false;
  return (style->Display() != blink::EDisplay::kNone &&
          style->Visibility() != blink::EVisibility::kHidden &&
          style->Opacity() != 0);
}
bool Element::NotStandardIsAutofilled() const {
  if (blink::IsHTMLFormControlElement(ConstUnwrap<blink::Element>()))
    return blink::ToHTMLFormControlElement(ConstUnwrap<blink::Element>())
        .IsAutofilled();
  return false;
}

}  // namespace webagents
