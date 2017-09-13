// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/css_style_declaration.h"

#include "core/dom/Element.h"
#include "core/style/ComputedStyle.h"
#include "core/css/ComputedStyleCSSValueMapping.h"
#include "core/css/CSSValue.h"
#include "public/platform/WebString.h"
#include "core/dom/NodeComputedStyle.h"

namespace webagents {

CSSStyleDeclaration::~CSSStyleDeclaration() {
  private_.Reset();
}

CSSStyleDeclaration::CSSStyleDeclaration(blink::Element& element)
    : private_(&element) {}

CSSStyleDeclaration::CSSStyleDeclaration(const CSSStyleDeclaration& style) {
  Assign(style);
}

void CSSStyleDeclaration::Assign(const CSSStyleDeclaration& style) {
  private_ = style.private_;
}

blink::WebString CSSStyleDeclaration::getPropertyValue(
    blink::WebString property) const {
  const blink::ComputedStyle* style = private_->EnsureComputedStyle();
  const blink::CSSValue* value = blink::ComputedStyleCSSValueMapping::Get(
      cssPropertyID(property), *style);
  if (value)
    return value->CssText();
  return "";
}

}  // namespace webagents
