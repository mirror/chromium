// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LayoutBoxModelObjectPainterInterface_h
#define LayoutBoxModelObjectPainterInterface_h

#include "core/paint/BoxModelObjectPainter.h"

namespace blink {

class LayoutBoxModelObject;

// Legacy layout implementation of the BoxModelObjectPaintInterface.
class LayoutBoxModelObjectPainterInterface
    : public virtual BoxModelPainterInterface {
 public:
  LayoutBoxModelObjectPainterInterface(const LayoutBoxModelObject& box_model)
      : box_model_(box_model) {}

  bool IsLayoutNG() const override { return false; }
  bool IsBox() const override { return false; }
  LayoutRectOutsets BorderOutsets(
      const BoxPainterBase::FillLayerInfo&) const override;
  LayoutRectOutsets PaddingOutsets(
      const BoxPainterBase::FillLayerInfo&) const override;
  const Document& GetDocument() const override;
  const ComputedStyle& Style() const override;
  LayoutRectOutsets BorderPaddingInsets() const override;
  bool HasOverflowClip() const override;
  Node* GeneratingNode() const override;
  operator const DisplayItemClient&() const override;
  const LayoutBoxModelObject* GetLayoutBoxModelObject() const override {
    return &box_model_;
  }

 private:
  const LayoutBoxModelObject& box_model_;
};

}  // namespace blink

#endif  // LayoutBoxModelObjectPainterInterface_h
