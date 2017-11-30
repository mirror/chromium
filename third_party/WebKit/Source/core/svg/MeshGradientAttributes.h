// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MeshGradientAttributes_h
#define MeshGradientAttributes_h

#include "core/svg/GradientAttributes.h"
#include "core/svg/SVGLength.h"
#include "platform/heap/Handle.h"

namespace blink {

struct MeshGradientAttributes : GradientAttributes {
  DISALLOW_NEW();

 public:
  MeshGradientAttributes()
    : x_(SVGLength::Create(SVGLengthMode::kWidth))
    , y_(SVGLength::Create(SVGLengthMode::kHeight))
    , x_set_(false)
    , y_set_(false) {}

  SVGLength* X() const { return x_.Get(); }
  SVGLength* Y() const { return y_.Get(); }
  bool HasX() const { return x_set_; }
  bool HasY() const { return y_set_; }

  void SetX(SVGLength* value) {
    x_ = value;
    x_set_ = true;
  }
  void SetY(SVGLength* value) {
    y_ = value;
    y_set_ = true;
  }

  void Trace(blink::Visitor* visitor) {
    visitor->Trace(x_);
    visitor->Trace(y_);
  }

 private:
  Member<SVGLength> x_;
  Member<SVGLength> y_;

  // Property states
  bool x_set_ : 1;
  bool y_set_ : 1;
};

// Wrapper object for the MeshGradientAttributes part object.
class MeshGradientAttributesWrapper
    : public GarbageCollectedFinalized<MeshGradientAttributesWrapper> {
 public:
  static MeshGradientAttributesWrapper* Create() {
    return new MeshGradientAttributesWrapper;
  }

  MeshGradientAttributes& Attributes() { return attributes_; }
  void Set(const MeshGradientAttributes& attributes) {
    attributes_ = attributes;
  }
  void Trace(blink::Visitor* visitor) { visitor->Trace(attributes_); }

 private:
  MeshGradientAttributesWrapper() = default;

  MeshGradientAttributes attributes_;
};

}  // namespace blink

#endif  // MeshGradientAttributes_h
