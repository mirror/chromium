// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/webagents/css_style_declaration.h"

#include "core/css/CSSStyleDeclaration.h"
#include "public/platform/WebString.h"

namespace webagents {

CSSStyleDeclaration::~CSSStyleDeclaration() {
  private_.Reset();
}

CSSStyleDeclaration::CSSStyleDeclaration(blink::CSSStyleDeclaration& style)
    : private_(&style) {}

CSSStyleDeclaration::CSSStyleDeclaration(const CSSStyleDeclaration& style) {
  Assign(style);
}

void CSSStyleDeclaration::Assign(const CSSStyleDeclaration& style) {
  private_ = style.private_;
}

blink::WebString CSSStyleDeclaration::getPropertyValue(
    blink::WebString property) const {
  return private_->getPropertyValue(property);
}

}  // namespace webagents
