// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BoxModelObjectPainter_h
#define BoxModelObjectPainter_h

#include "core/layout/BackgroundBleedAvoidance.h"
#include "core/paint/BoxPainterBase.h"
#include "core/paint/RoundedInnerRectClipper.h"
#include "platform/geometry/LayoutSize.h"
#include "platform/graphics/GraphicsTypes.h"
#include "platform/wtf/Allocator.h"

namespace blink {

class FillLayer;
class InlineFlowBox;
class LayoutRect;
struct PaintInfo;
class LayoutBoxModelObject;
class BackgroundImageGeometry;

class LayoutBox;
class BoxPainterInterface;

// Abstract interface between the layout and paint code for box model object
// painting.
class BoxModelPainterInterface {
 public:
  virtual bool IsLayoutNG() const = 0;
  virtual bool IsBox() const = 0;
  virtual LayoutRectOutsets BorderOutsets(
      const BoxPainterBase::FillLayerInfo&) const = 0;
  virtual LayoutRectOutsets PaddingOutsets(
      const BoxPainterBase::FillLayerInfo&) const = 0;
  virtual const Document& GetDocument() const = 0;
  virtual const ComputedStyle& Style() const = 0;
  virtual LayoutRectOutsets BorderPaddingInsets() const = 0;
  virtual bool HasOverflowClip() const = 0;
  virtual Node* GeneratingNode() const = 0;
  virtual const BoxPainterInterface* ToBoxPainterInterface() const {
    return nullptr;
  }
  virtual const LayoutBoxModelObject* GetLayoutBoxModelObject() const {
    return nullptr;
  }
  virtual operator const DisplayItemClient&() const = 0;
};

// Abstract interface between the layout and paint code for box painting.
class BoxPainterInterface : public virtual BoxModelPainterInterface {
 public:
  virtual LayoutSize LocationOffset() const = 0;
  virtual IntSize ScrollOffset() const = 0;
  virtual LayoutSize ScrollSize() const = 0;
  virtual LayoutRect OverflowClipRect(const LayoutPoint&) const = 0;
};

// BoxModelObjectPainter is a class that can paint either a LayoutBox or a
// LayoutInline and allows code sharing between block and inline block painting.
class BoxModelObjectPainter : public BoxPainterBase {
  STACK_ALLOCATED();

 public:
  BoxModelObjectPainter(const BoxModelPainterInterface& box_model)
      : box_model_(box_model) {}

  void PaintFillLayers(const PaintInfo&,
                       const Color&,
                       const FillLayer&,
                       const LayoutRect&,
                       BackgroundImageGeometry&,
                       BackgroundBleedAvoidance = kBackgroundBleedNone,
                       SkBlendMode = SkBlendMode::kSrcOver);

  void PaintFillLayer(const PaintInfo&,
                      const Color&,
                      const FillLayer&,
                      const LayoutRect&,
                      BackgroundBleedAvoidance,
                      BackgroundImageGeometry&,
                      SkBlendMode = SkBlendMode::kSrcOver,
                      const InlineFlowBox* = nullptr,
                      const LayoutSize& = LayoutSize());

  static bool IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
      const LayoutBoxModelObject*,
      const PaintInfo&);

  bool IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
      const PaintInfo&);

 private:
  const BoxModelPainterInterface& box_model_;
};

}  // namespace blink

#endif  // BoxModelObjectPainter_h
