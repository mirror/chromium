// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LayoutSVGResourceMeshGradient_h
#define LayoutSVGResourceMeshGradient_h

#include "core/layout/svg/LayoutSVGResourceGradient.h"
#include "core/svg/MeshGradientAttributes.h"

namespace blink {

class SVGMeshGradientElement;

class LayoutSVGResourceMeshGradient final : public LayoutSVGResourceGradient {
 public:
  explicit LayoutSVGResourceMeshGradient(SVGMeshGradientElement*);
  ~LayoutSVGResourceMeshGradient() override;

  const char* GetName() const override {
    return "LayoutSVGResourceMeshGradient";
  }

  LayoutSVGResourceType ResourceType() const override {
    return kMeshGradientResourceType;
  }

  FloatPoint StartPoint(const MeshGradientAttributes&) const;

 protected:
  SVGUnitTypes::SVGUnitType GradientUnits() const override {
    return Attributes().GradientUnits();
  }
  AffineTransform CalculateGradientTransform() const override {
    return Attributes().GradientTransform();
  }
  bool CollectGradientAttributes() override;
  scoped_refptr<Gradient> BuildGradient() const override;

 private:
  const MeshGradientAttributes& Attributes() const {
    return attributes_wrapper_->Attributes();
  }
  MeshGradientAttributes& MutableAttributes() {
    return attributes_wrapper_->Attributes();
  }

  Persistent<MeshGradientAttributesWrapper> attributes_wrapper_;
};

}  // namespace blink

#endif  // LayoutSVGResourceMeshGradient_h
