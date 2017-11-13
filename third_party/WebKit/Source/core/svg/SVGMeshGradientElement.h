// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SVGMeshGradientElement_h
#define SVGMeshGradientElement_h

#include "core/svg/SVGAnimatedLength.h"
#include "core/svg/SVGGradientElement.h"
#include "platform/heap/Handle.h"

namespace blink {

class SVGMeshGradientElement final : public SVGGradientElement {
  DEFINE_WRAPPERTYPEINFO();

 public:
  DECLARE_NODE_FACTORY(SVGMeshGradientElement);

  SVGAnimatedLength* x() const { return x_.Get(); }
  SVGAnimatedLength* y() const { return y_.Get(); }

  virtual void Trace(blink::Visitor*);

 protected:
  void SvgAttributeChanged(const QualifiedName&) override;

  LayoutObject* CreateLayoutObject(const ComputedStyle&) override;

  bool SelfHasRelativeLengths() const override;

 private:
  explicit SVGMeshGradientElement(Document&);

  Member<SVGAnimatedLength> x_;
  Member<SVGAnimatedLength> y_;
};

}  // namespace blink

#endif  // SVGMeshGradientElement_h
