// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BoxPainterBase_h
#define BoxPainterBase_h

#include "core/layout/BackgroundBleedAvoidance.h"
#include "core/style/StyleImage.h"
#include "platform/geometry/LayoutSize.h"
#include "platform/wtf/Allocator.h"
#include "third_party/skia/include/core/SkBlendMode.h"

namespace blink {

class BackgroundImageGeometry;
class ComputedStyle;
class DisplayItemClient;
class Document;
class FillLayer;
class FloatRoundedRect;
class ImageResourceObserver;
class LayoutPoint;
class LayoutRect;
class LayoutRectOutsets;
struct PaintInfo;

// Base class for box painting. Has no dependencies on the layout tree and thus
// provides functionality and definitions that can be shared between both legacy
// layout and LayoutNG. For performance reasons no virtual methods are utilized.
class BoxPainterBase {
  STACK_ALLOCATED();

 public:
  BoxPainterBase() {}

  void PaintFillLayer(const PaintInfo&,
                      const Color&,
                      const FillLayer&,
                      const LayoutRect&,
                      BackgroundBleedAvoidance,
                      BackgroundImageGeometry&,
                      SkBlendMode = SkBlendMode::kSrcOver);

  LayoutRect BoundsForDrawingRecorder(const PaintInfo&,
                                      const LayoutPoint& adjusted_paint_offset);

  static void PaintNormalBoxShadow(const PaintInfo&,
                                   const LayoutRect&,
                                   const ComputedStyle&,
                                   bool include_logical_left_edge = true,
                                   bool include_logical_right_edge = true);

  // The input rect should be the border rect. The outer bounds of the shadow
  // will be inset by border widths.
  static void PaintInsetBoxShadow(const PaintInfo&,
                                  const LayoutRect&,
                                  const ComputedStyle&,
                                  bool include_logical_left_edge = true,
                                  bool include_logical_right_edge = true);

  // This form is used by callers requiring special computation of the outer
  // bounds of the shadow. For example, TableCellPainter insets the bounds by
  // half widths of collapsed borders instead of the default whole widths.
  static void PaintInsetBoxShadowInBounds(
      const PaintInfo&,
      const FloatRoundedRect& bounds,
      const ComputedStyle&,
      bool include_logical_left_edge = true,
      bool include_logical_right_edge = true);

  static void PaintBorder(const ImageResourceObserver&,
                          const Document&,
                          Node*,
                          const PaintInfo&,
                          const LayoutRect&,
                          const ComputedStyle&,
                          BackgroundBleedAvoidance = kBackgroundBleedNone,
                          bool include_logical_left_edge = true,
                          bool include_logical_right_edge = true);

  static bool ShouldForceWhiteBackgroundForPrintEconomy(const Document&,
                                                        const ComputedStyle&);

  typedef Vector<const FillLayer*, 8> FillLayerOcclusionOutputList;
  // Returns true if the result fill layers have non-associative blending or
  // compositing mode.  (i.e. The rendering will be different without creating
  // isolation group by context.saveLayer().) Note that the output list will be
  // in top-bottom order.
  bool CalculateFillLayerOcclusionCulling(
      FillLayerOcclusionOutputList& reversed_paint_list,
      const FillLayer&,
      const Document&,
      const ComputedStyle&);

  struct FillLayerInfo {
    STACK_ALLOCATED();

   public:
    FillLayerInfo(const Document&,
                  const ComputedStyle&,
                  bool has_overflow_clip,
                  Color bg_color,
                  const FillLayer&,
                  BackgroundBleedAvoidance,
                  bool include_left_edge,
                  bool include_right_edge);

    // FillLayerInfo is a temporary, stack-allocated container which cannot
    // outlive the StyleImage.  This would normally be a raw pointer, if not for
    // the Oilpan tooling complaints.
    Member<StyleImage> image;
    Color color;

    bool include_left_edge;
    bool include_right_edge;
    bool is_bottom_layer;
    bool is_border_fill;
    bool is_clipped_with_local_scrolling;
    bool is_rounded_fill;
    bool should_paint_image;
    bool should_paint_color;
  };

  FloatRoundedRect BackgroundRoundedRectAdjustedForBleedAvoidance(
      const LayoutRect& border_rect,
      BackgroundBleedAvoidance,
      bool include_logical_left_edge,
      bool include_logical_right_edge) const;
  FloatRoundedRect RoundedBorderRectForClip(
      const FillLayerInfo&,
      const FillLayer&,
      const LayoutRect&,
      BackgroundBleedAvoidance,
      LayoutRectOutsets border_padding_insets) const;

 protected:
  void PaintFillLayerBackground(GraphicsContext&,
                                const BoxPainterBase::FillLayerInfo&,
                                Image*,
                                SkBlendMode,
                                const BackgroundImageGeometry&,
                                LayoutRect scrolled_paint_rect);
  LayoutRectOutsets BorderOutsets(const BoxPainterBase::FillLayerInfo&) const;
  LayoutRectOutsets PaddingOutsets(const BoxPainterBase::FillLayerInfo&) const;

  virtual FloatRoundedRect GetBackgroundRoundedRect(
      const LayoutRect& border_rect,
      bool include_logical_left_edge,
      bool include_logical_right_edge) const;
  virtual BoxPainterBase::FillLayerInfo GetFillLayerInfo(
      const Color&,
      const FillLayer&,
      BackgroundBleedAvoidance) const = 0;

  virtual Node* GetNode() const = 0;
  virtual const ComputedStyle& Style() const = 0;
  virtual const DisplayItemClient& DisplayItem() const = 0;
  virtual LayoutRectOutsets Border() const = 0;
  virtual LayoutRectOutsets Padding() const = 0;

  virtual void PaintFillLayerTextFillBox(GraphicsContext&,
                                         const BoxPainterBase::FillLayerInfo&,
                                         Image*,
                                         SkBlendMode composite_op,
                                         const BackgroundImageGeometry&,
                                         const LayoutRect&,
                                         LayoutRect scrolled_paint_rect) = 0;
  virtual LayoutRect AdjustForScrolledContent(
      const PaintInfo&,
      const BoxPainterBase::FillLayerInfo&,
      const LayoutRect&) = 0;
};

}  // namespace blink

#endif
