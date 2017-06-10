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

struct TableCellPainter::CollapsedBorderPaintInfo {
  // This will be set to null if we don't need to paint this border.
  const CollapsedBorderValue* value;
  int inner_width;
  int outer_width;
  // We may paint the border not in full length if the corner is covered by
  // another higher priority border. The following are the outsets from the
  // border box of the begin and end points of the painted segment.
  int begin_outset;
  int end_outset;
};

void TableCellPainter::AdjustBorderSegments(
    CollapsedBorderPaintInfo& start_border,
    CollapsedBorderPaintInfo& end_border,
    CollapsedBorderPaintInfo& before_border,
    CollapsedBorderPaintInfo& after_border) {
  if (!start_border.value->IsVisible())
    start_border.value = nullptr;
  if (!before_border.value->IsVisible())
    before_border.value = nullptr;
  if (!end_border.value->IsVisible())
    end_border.value = nullptr;
  if (!after_border.value->IsVisible())
    after_border.value = nullptr;

  if (start_border.value && before_border.value) {
    // First suppose both border cover the corner. Which is visible depends on
    // transparency and paint order. This is to keep the current behavior for
    // some layout tests e.g. css2.1/t170602-bdr-conflict-w-01-d.html, that we
    // paint overlapping borders at a corner if the borders have the same width.
    // TODO(crbug.com/672216): Determine the best way to deal with this.
    //
    // All the graphs below are examples of left-to-right and horizontal-tb
    // mode.  _____
    //       |_|___ before
    // start | |
    start_border.begin_outset = before_border.outer_width;
    before_border.begin_outset = start_border.outer_width;
    if (!start_border.value->CoversCorner(*before_border.value)) {
      //        _____
      //       |_____ before
      // start | |
      start_border.begin_outset = -before_border.inner_width;
    }
    if (!before_border.value->CoversCorner(*start_border.value)) {
      //        _____
      //       | |___ before
      // start | |
      before_border.begin_outset = -start_border.inner_width;
    }
  }

  if (end_border.value && before_border.value) {
    //        _____
    // before ___|_|
    //           | | end
    end_border.begin_outset = before_border.outer_width;
    before_border.end_outset = end_border.outer_width;
    if (!end_border.value->CoversCorner(*before_border.value)) {
      //        _____
      // before _____|
      //           | | end
      end_border.begin_outset = -before_border.inner_width;
    }
    if (!before_border.value->CoversCorner(*end_border.value)) {
      //        _____
      // before ___| |
      //           | | end
      before_border.end_outset = -end_border.inner_width;
    }
  }

  if (start_border.value && after_border.value) {
    // start |_|___
    //       |_|___ after
    start_border.end_outset = after_border.outer_width;
    after_border.begin_outset = start_border.outer_width;
    if (!start_border.value->CoversCorner(*after_border.value)) {
      // start |_|___
      //       |_____ after
      start_border.end_outset = -after_border.inner_width;
    }
    if (!after_border.value->CoversCorner(*start_border.value)) {
      // start | |___
      //       |_|___ after
      after_border.begin_outset = -start_border.inner_width;
    }
  }

  if (after_border.value && end_border.value) {
    //       ___|_| end
    // after ___|_|
    end_border.end_outset = after_border.outer_width;
    after_border.end_outset = end_border.outer_width;
    if (!after_border.value->CoversCorner(*end_border.value)) {
      //       ___| | end
      // after ___|_|
      after_border.end_outset = -end_border.inner_width;
    }
    if (!end_border.value->CoversCorner(*after_border.value)) {
      //       ___|_| end
      // after _____|
      end_border.end_outset = -after_border.inner_width;
    }
  }

  // Skip painting the start border if it will be painted by the preceding cell
  // as its end border.
  if (start_border.value) {
    const auto* cell_preceding = CellPreceding();
    if (cell_preceding && cell_preceding->RowIndex() == cell_.RowIndex() &&
        cell_preceding->RowSpan() >= cell_.RowSpan()) {
      start_border.value = nullptr;
      // Otherwise we'll still paint the shared border twice.
      // TODO(crbug.com/2902 etc.): Paint collapsed borders by grid cells.
    }
  }
  // Skip painting the before border if it will be painted by the above cell
  // as its after border.
  if (before_border.value) {
    const auto* cell_above = CellAbove();
    if (cell_above &&
        cell_above->AbsoluteColumnIndex() == cell_.AbsoluteColumnIndex() &&
        cell_above->ColSpan() >= cell_.ColSpan()) {
      before_border.value = nullptr;
      // Otherwise we'll still paint the shared border twice.
      // TODO(crbug.com/2902 etc.): Paint collapsed borders by grid cells.
    }
  }
}

static const LayoutTableCell::CollapsedBorderValues* GetCollapsedBorderValues(
    const LayoutTableCell* cell) {
  return cell ? cell->GetCollapsedBorderValues() : nullptr;
}

void TableCellPainter::AdjustEndBorderWithCellFollowing(
    CollapsedBorderPaintInfo& end_border) {
  const auto* cell_following = CellFollowing();
  const auto* borders_following = GetCollapsedBorderValues(cell_following);
  if (!borders_following ||
      // |cell_| doesn't share the start-before corner with |cell_following|
      // if they don't start from the same row.
      cell_following->RowIndex() != cell_.RowIndex())
    return;

  bool should_shorten = false;
  if (!end_border.value->CoversCorner(borders_following->BeforeBorder())) {
    // The corner is covered by the before border of |cell_following|.
    //        |_______
    // ======|________
    // cell_ | | cell_following
    //       | |
    should_shorten = true;
  } else if (const auto* borders_above_following =
                 GetCollapsedBorderValues(table_.CellAbove(*cell_following))) {
    if (end_border.value->CoversCorner(
            borders_above_following->StartBorder())) {
      // The corner is covered by |end_border|, but the border may need
      // lengthening to match the before border of |cell_following|.
      //        |_______
      // ======| |______
      // cell_ | | cell_following
      //       | |
      end_border.begin_outset = std::max(
          end_border.begin_outset,
          static_cast<int>(cell_following->CollapsedOuterBorderBefore()));
    } else {
      // The corner is covered by the start border of the cell above
      // |cell_following|.
      //      |   |______
      // =====|___|______
      // cell_ | | cell_following
      //       | |
      should_shorten = true;
    }
  }
  if (should_shorten) {
    end_border.begin_outset = std::min(
        end_border.begin_outset,
        -static_cast<int>(cell_following->CollapsedInnerBorderBefore()));
  }

  // If colspans don't equal, then this cell doesn't share the after-end corner
  // with |cell_following|.
  if (cell_.RowSpan() != cell_following->RowSpan())
    return;

  should_shorten = false;
  if (!end_border.value->CoversCorner(borders_following->AfterBorder())) {
    // The corner is covered by the after border of |cell_following|.
    // cell_ | | cell_following
    //       |_|______
    // ======|________
    //        |
    should_shorten = true;
  } else if (const auto* borders_below_following =
                 GetCollapsedBorderValues(table_.CellBelow(*cell_following))) {
    if (end_border.value->CoversCorner(
            borders_below_following->StartBorder())) {
      // The corner is covered by |end_border|, but may need lengthening to
      // match the before border of |cell_following|.
      // cell_ | | cell_following
      //       | |______
      // ======|_|______
      //        |
      end_border.end_outset = std::max(
          end_border.end_outset,
          static_cast<int>(cell_following->CollapsedOuterBorderBefore()));
    } else {
      // The corner is covered by the start border of the cell below
      // |cell_following|.
      // cell_ | | cell_following
      //       |_|_______
      // =====|   |______
      //      |   |
      should_shorten = true;
    }
  }
  if (should_shorten) {
    end_border.end_outset = std::min(
        end_border.end_outset,
        -static_cast<int>(cell_following->CollapsedInnerBorderAfter()));
  }
}

void TableCellPainter::AdjustAfterBorderWithCellBelow(
    CollapsedBorderPaintInfo& after_border) {
  const auto* cell_below = CellBelow();
  const auto* borders_below = GetCollapsedBorderValues(cell_below);
  if (!borders_below ||
      // |cell_| doesn't share the start-after corner with |cell_below| if they
      // don't start from the same column.
      cell_below->AbsoluteColumnIndex() != cell_.AbsoluteColumnIndex())
    return;

  bool should_shorten = false;
  if (!after_border.value->CoversCorner(borders_below->StartBorder())) {
    // The corner is covered by the start border of |cell_below|.
    //      | cell_
    //      |________
    // ====| |_______
    //     | |
    //     | | cell_below
    should_shorten = true;
  } else if (const auto* borders_preceding_below =
                 GetCollapsedBorderValues(table_.CellPreceding(*cell_below))) {
    if (after_border.value->CoversCorner(
            borders_preceding_below->BeforeBorder())) {
      // The corner is covered by |before_border|, but may need lengthening to
      // match the start border of |cell_below|.
      //                 |  cell_
      //                 |________
      //       =========|_________
      //                | |
      // preceding_below| | cell_below
      after_border.begin_outset =
          std::max(after_border.begin_outset,
                   static_cast<int>(cell_below->CollapsedOuterBorderStart()));
    } else {
      // The corner is covered by the before border of the cell preceding
      // |cell_below|.
      //     ___________|  cell_
      //                 |_______
      //   a wide border |_______
      //     ____________|
      // preceing_below| | cell_below
      should_shorten = true;
    }
  }
  if (should_shorten) {
    after_border.begin_outset =
        std::min(after_border.begin_outset,
                 -static_cast<int>(cell_below->CollapsedInnerBorderStart()));
  }

  // |cell_| doesn't share share the after-end corner with |cell_below| if
  // their rowspans don't equal.
  if (cell_.RowSpan() != cell_below->RowSpan())
    return;

  should_shorten = false;
  if (!after_border.value->CoversCorner(borders_below->EndBorder())) {
    // The corner is covered by the end border of |cell_below|.
    //      cell_  |
    //   __________|
    //   _________| |====
    //            | |
    // cell_below | |
    should_shorten = true;
  } else if (const auto* borders_following_below =
                 GetCollapsedBorderValues(table_.CellFollowing(*cell_below))) {
    if (after_border.value->CoversCorner(
            borders_following_below->BeforeBorder())) {
      // The corner is covered by |after_border|, but may need lengthening to
      // match the end border of |cell_below|.
      //      cell_  |
      //   __________|
      //   ___________|========
      //            | |
      // cell_below | | following_below
      after_border.end_outset =
          std::max(after_border.end_outset,
                   static_cast<int>(cell_below->CollapsedOuterBorderEnd()));
    } else {
      // The corner is covered by the before border of the cell following
      // |cell_below|.
      //     cell_   |_________
      //   _________|
      //   _________| a wide border
      //            |__________
      // cell_below | | following_below
      should_shorten = true;
    }
  }
  if (should_shorten) {
    after_border.end_outset =
        std::min(after_border.end_outset,
                 -static_cast<int>(cell_below->CollapsedInnerBorderEnd()));
  }
}

void TableCellPainter::AdjustForWritingModeAndDirection(
    CollapsedBorderPaintInfo& start_border,
    CollapsedBorderPaintInfo& end_border,
    CollapsedBorderPaintInfo& before_border,
    CollapsedBorderPaintInfo& after_border) {
  const auto& style = cell_.StyleForCellFlow();
  if (!style.IsLeftToRightDirection()) {
    std::swap(start_border, end_border);
    std::swap(before_border.begin_outset, before_border.end_outset);
    std::swap(after_border.begin_outset, after_border.end_outset);
  }
  if (!style.IsHorizontalWritingMode()) {
    std::swap(after_border, end_border);
    std::swap(before_border, start_border);
    if (style.IsFlippedBlocksWritingMode()) {
      std::swap(start_border, end_border);
      std::swap(before_border.begin_outset, before_border.end_outset);
      std::swap(after_border.begin_outset, after_border.end_outset);
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

  const auto* values = cell_.GetCollapsedBorderValues();
  if (!values)
    return;

  GraphicsContext& context = paint_info.context;
  const DisplayItemClient& client = DisplayItemClientForBorders();
  if (DrawingRecorder::UseCachedDrawingIfPossible(
          context, client, DisplayItem::kTableCellCollapsedBorders))
    return;

  CollapsedBorderPaintInfo start_border = {
      &values->StartBorder(), cell_.CollapsedInnerBorderStart(),
      cell_.CollapsedOuterBorderStart(), 0, 0};
  CollapsedBorderPaintInfo end_border = {&values->EndBorder(),
                                         cell_.CollapsedInnerBorderEnd(),
                                         cell_.CollapsedOuterBorderEnd(), 0, 0};
  CollapsedBorderPaintInfo before_border = {
      &values->BeforeBorder(), cell_.CollapsedInnerBorderBefore(),
      cell_.CollapsedOuterBorderBefore(), 0, 0};
  CollapsedBorderPaintInfo after_border = {
      &values->AfterBorder(), cell_.CollapsedInnerBorderAfter(),
      cell_.CollapsedOuterBorderAfter(), 0, 0};

  AdjustBorderSegments(start_border, end_border, before_border, after_border);
  if (end_border.value)
    AdjustEndBorderWithCellFollowing(end_border);
  if (after_border.value)
    AdjustAfterBorderWithCellBelow(after_border);
  AdjustForWritingModeAndDirection(start_border, end_border, before_border,
                                   after_border);
  // Now left=start_border, right=end_border, before_border=top,
  // after_border=bottom.

  // Collapsed borders are half inside and half outside of |rect|.
  IntRect rect = PixelSnappedIntRect(
      PaintRectNotIncludingVisualOverflow(adjusted_paint_offset));
  // |paint_rect| covers the whole collapsed borders.
  IntRect paint_rect = rect;
  paint_rect.Expand(
      IntRectOutsets(before_border.outer_width, end_border.outer_width,
                     after_border.outer_width, start_border.outer_width));
  DrawingRecorder recorder(context, client,
                           DisplayItem::kTableCellCollapsedBorders, paint_rect);

  // We never paint diagonals at the joins.  We simply let the border with the
  // highest precedence paint on top of borders with lower precedence.
  if (before_border.value) {
    ObjectPainter::DrawLineForBoxSide(
        context, rect.X() - before_border.begin_outset,
        rect.Y() - before_border.outer_width,
        rect.MaxX() + before_border.end_outset,
        rect.Y() + before_border.inner_width, kBSTop,
        before_border.value->GetColor(),
        CollapsedBorderStyle(before_border.value->Style()), 0, 0, true);
  }
  if (after_border.value) {
    ObjectPainter::DrawLineForBoxSide(
        context, rect.X() - after_border.begin_outset,
        rect.MaxY() - after_border.inner_width,
        rect.MaxX() + after_border.end_outset,
        rect.MaxY() + after_border.outer_width, kBSBottom,
        after_border.value->GetColor(),
        CollapsedBorderStyle(after_border.value->Style()), 0, 0, true);
  }
  if (start_border.value) {
    ObjectPainter::DrawLineForBoxSide(
        context, rect.X() - start_border.outer_width,
        rect.Y() - start_border.begin_outset,
        rect.X() + start_border.inner_width,
        rect.MaxY() + start_border.end_outset, kBSLeft,
        start_border.value->GetColor(),
        CollapsedBorderStyle(start_border.value->Style()), 0, 0, true);
  }
  if (end_border.value) {
    ObjectPainter::DrawLineForBoxSide(
        context, rect.MaxX() - end_border.inner_width,
        rect.Y() - end_border.begin_outset,
        rect.MaxX() + end_border.outer_width,
        rect.MaxY() + end_border.end_outset, kBSRight,
        end_border.value->GetColor(),
        CollapsedBorderStyle(end_border.value->Style()), 0, 0, true);
  }
}

void TableCellPainter::PaintContainerBackgroundBehindCell(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset,
    const LayoutObject& background_object) {
  DCHECK(background_object != cell_);

  if (cell_.Style()->Visibility() != EVisibility::kVisible)
    return;

  if (!table_.ShouldCollapseBorders() &&
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
        table_.ShouldCollapseBorders();
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
  const ComputedStyle& style = cell_.StyleRef();
  if (!table_.ShouldCollapseBorders() &&
      style.EmptyCells() == EEmptyCells::kHide && !cell_.FirstChild())
    return;

  bool needs_to_paint_border =
      style.HasBorderDecoration() && !table_.ShouldCollapseBorders();
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

  if (!table_.ShouldCollapseBorders() &&
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
