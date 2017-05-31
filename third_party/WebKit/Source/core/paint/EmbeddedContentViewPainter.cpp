// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/EmbeddedContentViewPainter.h"

#include "core/frame/EmbeddedContentView.h"
#include "core/layout/LayoutEmbeddedContentView.h"
#include "core/paint/BoxPainter.h"
#include "core/paint/LayoutObjectDrawingRecorder.h"
#include "core/paint/ObjectPainter.h"
#include "core/paint/PaintInfo.h"
#include "core/paint/PaintLayer.h"
#include "core/paint/ReplacedPainter.h"
#include "core/paint/RoundedInnerRectClipper.h"
#include "core/paint/ScrollableAreaPainter.h"
#include "core/paint/TransformRecorder.h"
#include "platform/wtf/Optional.h"

namespace blink {

bool EmbeddedContentViewPainter::IsSelected() const {
  SelectionState s = layout_embedded_content_view_.GetSelectionState();
  if (s == SelectionNone)
    return false;
  if (s == SelectionInside)
    return true;

  int selection_start, selection_end;
  std::tie(selection_start, selection_end) =
      layout_embedded_content_view_.SelectionStartEnd();
  if (s == SelectionStart)
    return selection_start == 0;

  int end = layout_embedded_content_view_.GetNode()->hasChildren()
                ? layout_embedded_content_view_.GetNode()->CountChildren()
                : 1;
  if (s == SelectionEnd)
    return selection_end == end;
  if (s == SelectionBoth)
    return selection_start == 0 && selection_end == end;

  DCHECK(0);
  return false;
}

void EmbeddedContentViewPainter::Paint(const PaintInfo& paint_info,
                                       const LayoutPoint& paint_offset) {
  ObjectPainter(layout_embedded_content_view_)
      .CheckPaintOffset(paint_info, paint_offset);
  LayoutPoint adjusted_paint_offset =
      paint_offset + layout_embedded_content_view_.Location();
  if (!ReplacedPainter(layout_embedded_content_view_)
           .ShouldPaint(paint_info, adjusted_paint_offset))
    return;

  LayoutRect border_rect(adjusted_paint_offset,
                         layout_embedded_content_view_.Size());

  if (layout_embedded_content_view_.HasBoxDecorationBackground() &&
      (paint_info.phase == kPaintPhaseForeground ||
       paint_info.phase == kPaintPhaseSelection)) {
    BoxPainter(layout_embedded_content_view_)
        .PaintBoxDecorationBackground(paint_info, adjusted_paint_offset);
  }

  if (paint_info.phase == kPaintPhaseMask) {
    BoxPainter(layout_embedded_content_view_)
        .PaintMask(paint_info, adjusted_paint_offset);
    return;
  }

  if (ShouldPaintSelfOutline(paint_info.phase)) {
    ObjectPainter(layout_embedded_content_view_)
        .PaintOutline(paint_info, adjusted_paint_offset);
  }

  if (paint_info.phase != kPaintPhaseForeground)
    return;

  if (layout_embedded_content_view_.GetEmbeddedContentView()) {
    // TODO(schenney) crbug.com/93805 Speculative release assert to verify that
    // the crashes we see in FrameViewBase painting are due to a destroyed
    // LayoutEmbeddedContentView object.
    CHECK(layout_embedded_content_view_.GetNode());
    Optional<RoundedInnerRectClipper> clipper;
    if (layout_embedded_content_view_.Style()->HasBorderRadius()) {
      if (border_rect.IsEmpty())
        return;

      FloatRoundedRect rounded_inner_rect =
          layout_embedded_content_view_.Style()->GetRoundedInnerBorderFor(
              border_rect,
              LayoutRectOutsets(
                  -(layout_embedded_content_view_.PaddingTop() +
                    layout_embedded_content_view_.BorderTop()),
                  -(layout_embedded_content_view_.PaddingRight() +
                    layout_embedded_content_view_.BorderRight()),
                  -(layout_embedded_content_view_.PaddingBottom() +
                    layout_embedded_content_view_.BorderBottom()),
                  -(layout_embedded_content_view_.PaddingLeft() +
                    layout_embedded_content_view_.BorderLeft())),
              true, true);
      clipper.emplace(layout_embedded_content_view_, paint_info, border_rect,
                      rounded_inner_rect, kApplyToDisplayList);
    }

    layout_embedded_content_view_.PaintContents(paint_info, paint_offset);
  }

  // Paint a partially transparent wash over selected EmbeddedContentViews.
  if (IsSelected() && !paint_info.IsPrinting() &&
      !LayoutObjectDrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, layout_embedded_content_view_,
          paint_info.phase)) {
    LayoutRect rect = layout_embedded_content_view_.LocalSelectionRect();
    rect.MoveBy(adjusted_paint_offset);
    IntRect selection_rect = PixelSnappedIntRect(rect);
    LayoutObjectDrawingRecorder drawing_recorder(
        paint_info.context, layout_embedded_content_view_, paint_info.phase,
        selection_rect);
    paint_info.context.FillRect(
        selection_rect,
        layout_embedded_content_view_.SelectionBackgroundColor());
  }

  if (layout_embedded_content_view_.CanResize()) {
    ScrollableAreaPainter(
        *layout_embedded_content_view_.Layer()->GetScrollableArea())
        .PaintResizer(paint_info.context,
                      RoundedIntPoint(adjusted_paint_offset),
                      paint_info.GetCullRect());
  }
}

void EmbeddedContentViewPainter::PaintContents(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  LayoutPoint adjusted_paint_offset =
      paint_offset + layout_embedded_content_view_.Location();

  EmbeddedContentView* embedded_content_view =
      layout_embedded_content_view_.GetEmbeddedContentView();
  CHECK(embedded_content_view);

  IntPoint paint_location(RoundedIntPoint(
      adjusted_paint_offset +
      layout_embedded_content_view_.ReplacedContentRect().Location()));

  // Views don't support painting with a paint offset, but instead
  // offset themselves using the frame rect location. To paint Views at
  // our desired location, we need to apply paint offset as a transform, with
  // the frame rect neutralized.
  IntSize view_paint_offset =
      paint_location - embedded_content_view->FrameRect().Location();
  TransformRecorder transform(
      paint_info.context, layout_embedded_content_view_,
      AffineTransform::Translation(view_paint_offset.Width(),
                                   view_paint_offset.Height()));
  CullRect adjusted_cull_rect(paint_info.GetCullRect(), -view_paint_offset);
  embedded_content_view->Paint(paint_info.context, adjusted_cull_rect);
}

}  // namespace blink
