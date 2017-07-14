// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/BoxPainter.h"

#include "core/html/HTMLFrameOwnerElement.h"
#include "core/layout/BackgroundBleedAvoidance.h"
#include "core/layout/LayoutBox.h"
#include "core/layout/LayoutObject.h"
#include "core/layout/LayoutTable.h"
#include "core/layout/LayoutTheme.h"
#include "core/layout/compositing/CompositedLayerMapping.h"
#include "core/paint/BackgroundImageGeometry.h"
#include "core/paint/BoxDecorationData.h"
#include "core/paint/LayoutObjectDrawingRecorder.h"
#include "core/paint/NinePieceImagePainter.h"
#include "core/paint/ObjectPainter.h"
#include "core/paint/PaintInfo.h"
#include "core/paint/ScrollRecorder.h"
#include "core/paint/ThemePainter.h"
#include "core/style/ShadowList.h"
#include "platform/LengthFunctions.h"
#include "platform/geometry/LayoutPoint.h"
#include "platform/graphics/GraphicsContextStateSaver.h"
#include "platform/wtf/Optional.h"

namespace blink {

BoxPainter::BoxPainter(const LayoutBox& layout_box)
    : BoxModelObjectPainter(layout_box) {}

void BoxPainter::Paint(const PaintInfo& paint_info,
                       const LayoutPoint& paint_offset) {
  ObjectPainter(GetLayoutBox()).CheckPaintOffset(paint_info, paint_offset);
  // Default implementation. Just pass paint through to the children.
  PaintChildren(paint_info, paint_offset + GetLayoutBox().Location());
}

void BoxPainter::PaintChildren(const PaintInfo& paint_info,
                               const LayoutPoint& paint_offset) {
  PaintInfo child_info(paint_info);
  for (LayoutObject* child = GetLayoutBox().SlowFirstChild(); child;
       child = child->NextSibling())
    child->Paint(child_info, paint_offset);
}

void BoxPainter::PaintBoxDecorationBackground(const PaintInfo& paint_info,
                                              const LayoutPoint& paint_offset) {
  LayoutRect paint_rect;
  Optional<ScrollRecorder> scroll_recorder;
  if (IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
          &GetLayoutBox(), paint_info)) {
    // For the case where we are painting the background into the scrolling
    // contents layer of a composited scroller we need to include the entire
    // overflow rect.
    paint_rect = GetLayoutBox().LayoutOverflowRect();
    scroll_recorder.emplace(paint_info.context, GetLayoutBox(),
                            paint_info.phase,
                            GetLayoutBox().ScrolledContentOffset());

    // The background painting code assumes that the borders are part of the
    // paintRect so we expand the paintRect by the border size when painting the
    // background into the scrolling contents layer.
    paint_rect.Expand(GetLayoutBox().BorderBoxOutsets());
  } else {
    paint_rect = GetLayoutBox().BorderBoxRect();
  }

  paint_rect.MoveBy(paint_offset);
  PaintBoxDecorationBackgroundWithRect(paint_info, paint_offset, paint_rect);
}

LayoutRect BoxPainter::BoundsForDrawingRecorder(
    const PaintInfo& paint_info,
    const LayoutPoint& adjusted_paint_offset) {
  LayoutRect bounds =
      IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
          &GetLayoutBox(), paint_info)
          ? GetLayoutBox().LayoutOverflowRect()
          : GetLayoutBox().SelfVisualOverflowRect();
  bounds.MoveBy(adjusted_paint_offset);
  return bounds;
}

void BoxPainter::PaintBoxDecorationBackgroundWithRect(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset,
    const LayoutRect& paint_rect) {
  bool painting_overflow_contents =
      IsPaintingBackgroundOfPaintContainerIntoScrollingContentsLayer(
          &GetLayoutBox(), paint_info);
  Optional<DisplayItemCacheSkipper> cache_skipper;
  // Disable cache in under-invalidation checking mode for MediaSliderPart
  // because we always paint using the latest data (buffered ranges, current
  // time and duration) which may be different from the cached data.
  if ((RuntimeEnabledFeatures::PaintUnderInvalidationCheckingEnabled() &&
       Style().Appearance() == kMediaSliderPart)
      // We may paint a delayed-invalidation object before it's actually
      // invalidated. Note this would be handled for us by
      // LayoutObjectDrawingRecorder but we have to use DrawingRecorder as we
      // may use the scrolling contents layer as DisplayItemClient below.
      || GetLayoutBox().FullPaintInvalidationReason() ==
             PaintInvalidationReason::kDelayedFull) {
    cache_skipper.emplace(paint_info.context);
  }

  const DisplayItemClient& display_item_client =
      painting_overflow_contents ? static_cast<const DisplayItemClient&>(
                                       *GetLayoutBox()
                                            .Layer()
                                            ->GetCompositedLayerMapping()
                                            ->ScrollingContentsLayer())
                                 : GetLayoutBox();
  if (DrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, display_item_client,
          DisplayItem::kBoxDecorationBackground))
    return;

  DrawingRecorder recorder(
      paint_info.context, display_item_client,
      DisplayItem::kBoxDecorationBackground,
      FloatRect(BoundsForDrawingRecorder(paint_info, paint_offset)));
  BoxDecorationData box_decoration_data(GetLayoutBox());
  GraphicsContextStateSaver state_saver(paint_info.context, false);

  if (!painting_overflow_contents) {
    // FIXME: Should eventually give the theme control over whether the box
    // shadow should paint, since controls could have custom shadows of their
    // own.
    PaintNormalBoxShadow(paint_info, paint_rect, Style());

    if (BleedAvoidanceIsClipping(box_decoration_data.bleed_avoidance)) {
      state_saver.Save();
      FloatRoundedRect border = Style().GetRoundedBorderFor(paint_rect);
      paint_info.context.ClipRoundedRect(border);

      if (box_decoration_data.bleed_avoidance == kBackgroundBleedClipLayer)
        paint_info.context.BeginLayer();
    }
  }

  // If we have a native theme appearance, paint that before painting our
  // background.  The theme will tell us whether or not we should also paint the
  // CSS background.
  IntRect snapped_paint_rect(PixelSnappedIntRect(paint_rect));
  ThemePainter& theme_painter = LayoutTheme::GetTheme().Painter();
  bool theme_painted =
      box_decoration_data.has_appearance &&
      !theme_painter.Paint(GetLayoutBox(), paint_info, snapped_paint_rect);
  bool should_paint_background =
      !theme_painted && (!paint_info.SkipRootBackground() ||
                         paint_info.PaintContainer() != &GetLayoutBox());
  if (should_paint_background) {
    PaintBackground(paint_info, paint_rect,
                    box_decoration_data.background_color,
                    box_decoration_data.bleed_avoidance);

    if (box_decoration_data.has_appearance)
      theme_painter.PaintDecorations(GetLayoutBox(), paint_info,
                                     snapped_paint_rect);
  }

  if (!painting_overflow_contents) {
    PaintInsetBoxShadow(paint_info, paint_rect, Style());

    // The theme will tell us whether or not we should also paint the CSS
    // border.
    if (box_decoration_data.has_border_decoration &&
        (!box_decoration_data.has_appearance ||
         (!theme_painted &&
          LayoutTheme::GetTheme().Painter().PaintBorderOnly(
              GetLayoutBox(), paint_info, snapped_paint_rect))) &&
        !(GetLayoutBox().IsTable() &&
          ToLayoutTable(&GetLayoutBox())->ShouldCollapseBorders())) {
      PaintBorder(GetLayoutBox(), GetDocument(), GetNode(), paint_info,
                  paint_rect, Style(), box_decoration_data.bleed_avoidance);
    }
  }

  if (box_decoration_data.bleed_avoidance == kBackgroundBleedClipLayer)
    paint_info.context.EndLayer();
}

void BoxPainter::PaintBackground(const PaintInfo& paint_info,
                                 const LayoutRect& paint_rect,
                                 const Color& background_color,
                                 BackgroundBleedAvoidance bleed_avoidance) {
  if (GetLayoutBox().IsDocumentElement())
    return;
  if (GetLayoutBox().BackgroundStolenForBeingBody())
    return;
  if (GetLayoutBox().BackgroundIsKnownToBeObscured())
    return;
  BackgroundImageGeometry geometry(GetLayoutBox());
  PaintFillLayers(paint_info, background_color,
                  GetLayoutBox().Style()->BackgroundLayers(), paint_rect,
                  geometry, bleed_avoidance);
}

void BoxPainter::PaintMask(const PaintInfo& paint_info,
                           const LayoutPoint& paint_offset) {
  if (GetLayoutBox().Style()->Visibility() != EVisibility::kVisible ||
      paint_info.phase != kPaintPhaseMask)
    return;

  if (LayoutObjectDrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, GetLayoutBox(), paint_info.phase))
    return;

  LayoutRect visual_overflow_rect(GetLayoutBox().VisualOverflowRect());
  visual_overflow_rect.MoveBy(paint_offset);

  LayoutObjectDrawingRecorder recorder(paint_info.context, GetLayoutBox(),
                                       paint_info.phase, visual_overflow_rect);
  LayoutRect paint_rect = LayoutRect(paint_offset, GetLayoutBox().Size());
  PaintMaskImages(paint_info, paint_rect);
}

void BoxPainter::PaintMaskImages(const PaintInfo& paint_info,
                                 const LayoutRect& paint_rect) {
  // Figure out if we need to push a transparency layer to render our mask.
  bool push_transparency_layer = false;
  bool flatten_compositing_layers =
      paint_info.GetGlobalPaintFlags() & kGlobalPaintFlattenCompositingLayers;
  bool mask_blending_applied_by_compositor =
      !flatten_compositing_layers && GetLayoutBox().HasLayer() &&
      GetLayoutBox().Layer()->MaskBlendingAppliedByCompositor();

  bool all_mask_images_loaded = true;

  if (!mask_blending_applied_by_compositor) {
    push_transparency_layer = true;
    StyleImage* mask_box_image =
        GetLayoutBox().Style()->MaskBoxImage().GetImage();
    const FillLayer& mask_layers = GetLayoutBox().Style()->MaskLayers();

    // Don't render a masked element until all the mask images have loaded, to
    // prevent a flash of unmasked content.
    if (mask_box_image)
      all_mask_images_loaded &= mask_box_image->IsLoaded();

    all_mask_images_loaded &= mask_layers.ImagesAreLoaded();

    paint_info.context.BeginLayer(1, SkBlendMode::kDstIn);
  }

  if (all_mask_images_loaded) {
    BackgroundImageGeometry geometry(GetLayoutBox());
    PaintFillLayers(paint_info, Color::kTransparent,
                    GetLayoutBox().Style()->MaskLayers(), paint_rect, geometry);
    NinePieceImagePainter::Paint(paint_info.context, GetLayoutBox(),
                                 GetDocument(), GetNode(), paint_rect, Style(),
                                 Style().MaskBoxImage());
  }

  if (push_transparency_layer)
    paint_info.context.EndLayer();
}

void BoxPainter::PaintClippingMask(const PaintInfo& paint_info,
                                   const LayoutPoint& paint_offset) {
  DCHECK(paint_info.phase == kPaintPhaseClippingMask);

  if (GetLayoutBox().Style()->Visibility() != EVisibility::kVisible)
    return;

  if (!GetLayoutBox().Layer() ||
      GetLayoutBox().Layer()->GetCompositingState() != kPaintsIntoOwnBacking)
    return;

  if (LayoutObjectDrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, GetLayoutBox(), paint_info.phase))
    return;

  IntRect paint_rect =
      PixelSnappedIntRect(LayoutRect(paint_offset, GetLayoutBox().Size()));
  LayoutObjectDrawingRecorder drawing_recorder(
      paint_info.context, GetLayoutBox(), paint_info.phase, paint_rect);
  paint_info.context.FillRect(paint_rect, Color::kBlack);
}

const LayoutBox& BoxPainter::GetLayoutBox() const {
  return static_cast<const LayoutBox&>(box_model_);
}

}  // namespace blink
