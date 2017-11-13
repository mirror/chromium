// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/svg/SVGMeshGradientElement.h"
#include "core/svg/SVGLength.h"

namespace blink {

inline SVGMeshGradientElement::SVGMeshGradientElement(Document& document)
    : SVGGradientElement(SVGNames::meshGradientTag, document),
      x_(SVGAnimatedLength::Create(this,
                                   SVGNames::x1Attr,
                                   SVGLength::Create(SVGLengthMode::kWidth))),
      y_(SVGAnimatedLength::Create(this,
                                   SVGNames::y1Attr,
                                   SVGLength::Create(SVGLengthMode::kHeight))) {

  // Initial values.
  x_->SetDefaultValueAsString("0%");
  y_->SetDefaultValueAsString("0%");

  AddToPropertyMap(x_);
  AddToPropertyMap(y_);
}

DEFINE_NODE_FACTORY(SVGMeshGradientElement)

void SVGMeshGradientElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(x_);
  visitor->Trace(y_);
  SVGGradientElement::Trace(visitor);
}

void SVGMeshGradientElement::SvgAttributeChanged(
    const QualifiedName& attr_name) {
    // TODO
}

LayoutObject* SVGMeshGradientElement::CreateLayoutObject(
    const ComputedStyle&) {
  // TODO
  return nullptr;
}

bool SVGMeshGradientElement::SelfHasRelativeLengths() const {
  return x_->CurrentValue()->IsRelative() || y_->CurrentValue()->IsRelative();
}

}  // namespace blink
