// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/svg/LayoutSVGResourceMeshGradient.h"
#include "core/svg/MeshGradientAttributes.h"
#include "core/svg/SVGMeshGradientElement.h"
#include "core/svg/SVGLength.h"

namespace blink {

inline SVGMeshGradientElement::SVGMeshGradientElement(Document& document)
    : SVGGradientElement(SVGNames::meshgradientTag, document),
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
  if (attr_name == SVGNames::xAttr || attr_name == SVGNames::yAttr) {
      SVGElement::InvalidationGuard invalidation_guard(this);

      UpdateRelativeLengthsInformation();

      if (auto* layout_object = ToLayoutSVGResourceContainer(this->GetLayoutObject())) {
        layout_object->InvalidateCacheAndMarkForLayout();
      }

      return;
    }

    SVGGradientElement::SvgAttributeChanged(attr_name);
}

LayoutObject* SVGMeshGradientElement::CreateLayoutObject(
    const ComputedStyle&) {
  return new LayoutSVGResourceMeshGradient(this);
}

namespace {

void SetGradientAttributes(const SVGGradientElement& element,
                           MeshGradientAttributes& attrs) {
  element.SynchronizeAnimatedSVGAttribute(AnyQName());
  element.CollectCommonAttributes(attrs);

  if (!IsSVGMeshGradientElement(element))
    return;

  const SVGMeshGradientElement& mesh = ToSVGMeshGradientElement(element);

  if (!attrs.HasX() && mesh.x()->IsSpecified())
    attrs.SetX(mesh.x()->CurrentValue());

  if (!attrs.HasY() && mesh.y()->IsSpecified())
    attrs.SetY(mesh.y()->CurrentValue());
}

}  // namespace

bool SVGMeshGradientElement::CollectGradientAttributes(
    MeshGradientAttributes& attrs) {
  DCHECK(GetLayoutObject());

  VisitedSet visited;
  const SVGGradientElement* current = this;

  while (true) {
    SetGradientAttributes(*current, attrs);
    visited.insert(current);

    current = current->ReferencedElement();
    if (!current || visited.Contains(current))
      break;
    if (!current->GetLayoutObject())
      return false;
  }

  return true;
}

bool SVGMeshGradientElement::SelfHasRelativeLengths() const {
  return x_->CurrentValue()->IsRelative() || y_->CurrentValue()->IsRelative();
}

}  // namespace blink
