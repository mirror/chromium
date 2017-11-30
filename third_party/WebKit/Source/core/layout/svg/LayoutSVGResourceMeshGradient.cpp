// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/svg/LayoutSVGResourceMeshGradient.h"

#include "core/svg/SVGMeshGradientElement.h"
#include "platform/graphics/Gradient.h"

namespace blink {

LayoutSVGResourceMeshGradient::LayoutSVGResourceMeshGradient(
    SVGMeshGradientElement* node)
    : LayoutSVGResourceGradient(node),
      attributes_wrapper_(MeshGradientAttributesWrapper::Create()) {}

LayoutSVGResourceMeshGradient::~LayoutSVGResourceMeshGradient() {}

bool LayoutSVGResourceMeshGradient::CollectGradientAttributes() {
  DCHECK(GetElement());
  attributes_wrapper_->Set(MeshGradientAttributes());
  return ToSVGMeshGradientElement(GetElement())
      ->CollectGradientAttributes(MutableAttributes());
}

FloatPoint LayoutSVGResourceMeshGradient::StartPoint(
    const MeshGradientAttributes& attributes) const {
  return SVGLengthContext::ResolvePoint(GetElement(),
                                        attributes.GradientUnits(),
                                        *attributes.X(), *attributes.Y());
}

scoped_refptr<Gradient> LayoutSVGResourceMeshGradient::BuildGradient() const {
  // TODO(fmalita)

  // dummy pretty pink for now.
  scoped_refptr<Gradient> gradient =
        Gradient::CreateLinear(FloatPoint(0, 0),
                               FloatPoint(0, 1),
                               kSpreadMethodPad,
                               Gradient::ColorInterpolation::kUnpremultiplied);
  gradient->AddColorStop({0, Color(0xffff69b4)});
  gradient->AddColorStop({1, Color(0xffff69b4)});

  return gradient;
}

}  // namespace blink
