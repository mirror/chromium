// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/TableCellPainter.h"

#include "core/layout/LayoutTableCell.h"
#include "core/paint/BlockPainter.h"
#include "core/paint/BoxPainter.h"
#include "core/paint/LayoutObjectDrawingRecorder.h"
#include "core/paint/ObjectPainter.h"
#include "core/paint/PaintInfo.h"
#include "platform/graphics/GraphicsContextStateSaver.h"
#include "platform/graphics/paint/DrawingRecorder.h"

namespace blink {

struct CollapsedBorderPaintInfo {
  const CollapsedBorderValue* value;
  int inner_width;
  int outer_width;
  // We may paint the border not in full length if the corner is covered by
  // another higher priority border. The following are the outsets from the
  // border box of the begin and end points of the painted segment.
  int begin_outset;
  int end_outset;
};

static void AdjustBorderSegments(CollapsedBorderPaintInfo& start,
                                 CollapsedBorderPaintInfo& end,
                                 CollapsedBorderPaintInfo& before,
                                 CollapsedBorderPaintInfo& after) {
  if (start.value->Compare(*before.value)) {
    // All the graphs are examples of left-to-right and horizontal-tb mode.
    //        _____
    //       |_____ before
    // start | |
    start.begin_outset = -before.inner_width;
    before.begin_outset = start.outer_width;
  } else {
    //        _____
    //       | |___ before
    // start | |
    start.begin_outset = before.outer_width;
    before.begin_outset = -start.inner_width;
  }

  if (end.value->Compare(*before.value)) {
    //        _____
    // before _____|
    //           | | end
    end.begin_outset = -before.inner_width;
    before.end_outset = end.outer_width;
  } else {
    //        _____
    // before ___| |
    //           | | end
    end.begin_outset = before.outer_width;
    before.end_outset = -end.inner_width;
  }

  if (start.value->Compare(*after.value)) {
    // start |_|___
    //       |_____ after
    start.end_outset = -after.inner_width;
    after.begin_outset = start.outer_width;
  } else {
    // start | |___
    //       |_|___ after
    start.end_outset = after.outer_width;
    after.begin_outset = -start.inner_width;
  }

  if (end.value->Compare(*after.value)) {
    //       ___|_| end
    // after _____|
    end.end_outset = -after.inner_width;
    after.end_outset = end.outer_width;
  } else {
    //       ___| | end
    // after ___|_|
    end.end_outset = after.outer_width;
    after.end_outset = -end.inner_width;
  }
}

static void AdjustForWritingModeAndDirection(const ComputedStyle& style,
                                             CollapsedBorderPaintInfo& start,
                                             CollapsedBorderPaintInfo& end,
                                             CollapsedBorderPaintInfo& before,
                                             CollapsedBorderPaintInfo& after) {
  if (!style.IsLeftToRightDirection()) {
    std::swap(start, end);
    std::swap(before.begin_outset, before.end_outset);
    std::swap(after.begin_outset, after.end_outset);
  }
  if (!style.IsHorizontalWritingMode()) {
    std::swap(after, end);
    std::swap(before, start);
    if (style.IsFlippedBlocksWritingMode()) {
      std::swap(start, end);
      std::swap(before.begin_outset, before.end_outset);
      std::swap(after.begin_outset, after.end_outset);
    }
  }
}

void TableCellPainter::Paint(const PaintInfo& paint_info,
                             const LayoutPoint& paint_offset) {
  BlockPainter(cell_).Paint(paint_info, paint_offset);
}

static EBorderStyle CollapsedBorderStyle(EBorderStyle style) {
  if (style == EBorderStyle::kOutset)
    return EBorderStyle::kGroove;
  if (style == EBorderStyle::kInset)
    return EBorderStyle::kRidge;
  return style;
}

const DisplayItemClient& TableCellPainter::DisplayItemClientForBorders() const {
  // TODO(wkorman): We may need to handle PaintInvalidationDelayedFull.
  // http://crbug.com/657186
  return cell_.UsesCompositedCellDisplayItemClients()
             ? static_cast<const DisplayItemClient&>(
                   *cell_.GetCollapsedBorderValues())
             : cell_;
}

void TableCellPainter::PaintCollapsedBorders(const PaintInfo& paint_info,
                                             const LayoutPoint& paint_offset) {
  if (cell_.Style()->Visibility() != EVisibility::kVisible)
    return;

  LayoutPoint adjusted_paint_offset = paint_offset + cell_.Location();
  if (!BlockPainter(cell_).IntersectsPaintRect(paint_info,
                                               adjusted_paint_offset))
    return;

  const LayoutTableCell::CollapsedBorderValues* values =
      cell_.GetCollapsedBorderValues();
  if (!values)
    return;

  GraphicsContext& context = paint_info.context;
  const DisplayItemClient& client = DisplayItemClientForBorders();
  if (DrawingRecorder::UseCachedDrawingIfPossible(
          context, client, DisplayItem::kTableCellCollapsedBorders))
    return;

  CollapsedBorderPaintInfo start = {&values->StartBorder(),
                                    cell_.CollapsedInnerBorderStart(),
                                    cell_.CollapsedOuterBorderStart(), 0, 0};
  CollapsedBorderPaintInfo end = {&values->EndBorder(),
                                  cell_.CollapsedInnerBorderEnd(),
                                  cell_.CollapsedOuterBorderEnd(), 0, 0};
  CollapsedBorderPaintInfo before = {&values->BeforeBorder(),
                                     cell_.CollapsedInnerBorderBefore(),
                                     cell_.CollapsedOuterBorderBefore(), 0, 0};
  CollapsedBorderPaintInfo after = {&values->AfterBorder(),
                                    cell_.CollapsedInnerBorderAfter(),
                                    cell_.CollapsedOuterBorderAfter(), 0, 0};

  AdjustBorderSegments(start, end, before, after);
  AdjustForWritingModeAndDirection(cell_.StyleForCellFlow(), start, end, before,
                                   after);
  // Now left=start, right=end, before=top, after=bottom.

  // Collapsed borders are half inside and half outside of |rect|.
  IntRect rect = PixelSnappedIntRect(
      PaintRectNotIncludingVisualOverflow(adjusted_paint_offset));
  // |paint_rect| covers the whole collapsed borders.
  IntRect paint_rect = rect;
  paint_rect.Expand(IntRectOutsets(before.outer_width, end.outer_width,
                                   after.outer_width, start.outer_width));
  DrawingRecorder recorder(context, client,
                           DisplayItem::kTableCellCollapsedBorders, paint_rect);

  // We never paint diagonals at the joins.  We simply let the border with the
  // highest precedence paint on top of borders with lower precedence.
  ObjectPainter::DrawLineForBoxSide(
      context, rect.X() - before.begin_outset, rect.Y() - before.outer_width,
      rect.MaxX() + before.end_outset, rect.Y() + before.inner_width, kBSTop,
      before.value->GetColor(), CollapsedBorderStyle(before.value->Style()), 0,
      0, true);
  ObjectPainter::DrawLineForBoxSide(
      context, rect.X() - after.begin_outset, rect.MaxY() - after.inner_width,
      rect.MaxX() + after.end_outset, rect.MaxY() + after.outer_width,
      kBSBottom, after.value->GetColor(),
      CollapsedBorderStyle(after.value->Style()), 0, 0, true);
  ObjectPainter::DrawLineForBoxSide(
      context, rect.X() - start.outer_width, rect.Y() - start.begin_outset,
      rect.X() + start.inner_width, rect.MaxY() + start.end_outset, kBSLeft,
      start.value->GetColor(), CollapsedBorderStyle(start.value->Style()), 0, 0,
      true);
  ObjectPainter::DrawLineForBoxSide(
      context, rect.MaxX() - end.inner_width, rect.Y() - end.begin_outset,
      rect.MaxX() + end.outer_width, rect.MaxY() + end.end_outset, kBSRight,
      end.value->GetColor(), CollapsedBorderStyle(end.value->Style()), 0, 0,
      true);
}

void TableCellPainter::PaintContainerBackgroundBehindCell(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset,
    const LayoutObject& background_object) {
  DCHECK(background_object != cell_);

  if (cell_.Style()->Visibility() != EVisibility::kVisible)
    return;

  LayoutTable* table = cell_.Table();
  if (!table->ShouldCollapseBorders() &&
      cell_.Style()->EmptyCells() == EEmptyCells::kHide && !cell_.FirstChild())
    return;

  LayoutRect paint_rect =
      PaintRectNotIncludingVisualOverflow(paint_offset + cell_.Location());
  PaintBackground(paint_info, paint_rect, background_object);
}

void TableCellPainter::PaintBackground(const PaintInfo& paint_info,
                                       const LayoutRect& paint_rect,
                                       const LayoutObject& background_object) {
  if (cell_.BackgroundStolenForBeingBody())
    return;

  Color c = background_object.ResolveColor(CSSPropertyBackgroundColor);
  const FillLayer& bg_layer = background_object.StyleRef().BackgroundLayers();
  if (bg_layer.HasImage() || c.Alpha()) {
    // We have to clip here because the background would paint
    // on top of the borders otherwise.  This only matters for cells and rows.
    bool should_clip =
        background_object.HasLayer() &&
        (background_object == cell_ || background_object == cell_.Parent()) &&
        cell_.Table()->ShouldCollapseBorders();
    GraphicsContextStateSaver state_saver(paint_info.context, should_clip);
    if (should_clip) {
      LayoutRect clip_rect(paint_rect.Location(), cell_.Size());
      clip_rect.Expand(cell_.BorderInsets());
      paint_info.context.Clip(PixelSnappedIntRect(clip_rect));
    }
    BoxPainter(cell_).PaintFillLayers(
        paint_info, c, bg_layer, paint_rect, kBackgroundBleedNone,
        SkBlendMode::kSrcOver, &background_object);
  }
}

void TableCellPainter::PaintBoxDecorationBackground(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  LayoutTable* table = cell_.Table();
  const ComputedStyle& style = cell_.StyleRef();
  if (!table->ShouldCollapseBorders() &&
      style.EmptyCells() == EEmptyCells::kHide && !cell_.FirstChild())
    return;

  bool needs_to_paint_border =
      style.HasBorderDecoration() && !table->ShouldCollapseBorders();
  if (!style.HasBackground() && !style.BoxShadow() && !needs_to_paint_border)
    return;

  if (LayoutObjectDrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, cell_, DisplayItem::kBoxDecorationBackground))
    return;

  LayoutRect visual_overflow_rect = cell_.VisualOverflowRect();
  visual_overflow_rect.MoveBy(paint_offset);
  // TODO(chrishtr): the pixel-snapping here is likely incorrect.
  LayoutObjectDrawingRecorder recorder(
      paint_info.context, cell_, DisplayItem::kBoxDecorationBackground,
      PixelSnappedIntRect(visual_overflow_rect));

  LayoutRect paint_rect = PaintRectNotIncludingVisualOverflow(paint_offset);

  BoxPainter::PaintNormalBoxShadow(paint_info, paint_rect, style);
  PaintBackground(paint_info, paint_rect, cell_);
  // TODO(wangxianzhu): Calculate the inset shadow bounds by insetting paintRect
  // by half widths of collapsed borders.
  BoxPainter::PaintInsetBoxShadow(paint_info, paint_rect, style);

  if (!needs_to_paint_border)
    return;

  BoxPainter::PaintBorder(cell_, paint_info, paint_rect, style);
}

void TableCellPainter::PaintMask(const PaintInfo& paint_info,
                                 const LayoutPoint& paint_offset) {
  if (cell_.Style()->Visibility() != EVisibility::kVisible ||
      paint_info.phase != kPaintPhaseMask)
    return;

  LayoutTable* table_elt = cell_.Table();
  if (!table_elt->ShouldCollapseBorders() &&
      cell_.Style()->EmptyCells() == EEmptyCells::kHide && !cell_.FirstChild())
    return;

  if (LayoutObjectDrawingRecorder::UseCachedDrawingIfPossible(
          paint_info.context, cell_, paint_info.phase))
    return;

  LayoutRect paint_rect = PaintRectNotIncludingVisualOverflow(paint_offset);
  LayoutObjectDrawingRecorder recorder(paint_info.context, cell_,
                                       paint_info.phase, paint_rect);
  BoxPainter(cell_).PaintMaskImages(paint_info, paint_rect);
}

LayoutRect TableCellPainter::PaintRectNotIncludingVisualOverflow(
    const LayoutPoint& paint_offset) {
  return LayoutRect(paint_offset, LayoutSize(cell_.PixelSnappedSize()));
}

}  // namespace blink
