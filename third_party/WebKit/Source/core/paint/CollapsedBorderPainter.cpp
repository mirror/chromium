// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/CollapsedBorderPainter.h"

#include "core/paint/BlockPainter.h"
#include "core/paint/LayoutObjectDrawingRecorder.h"
#include "core/paint/ObjectPainter.h"
#include "core/paint/PaintInfo.h"

namespace blink {

void CollapsedBorderPainter::SetupBorders() {
  const auto* values = cell_.GetCollapsedBorderValues();
  DCHECK(values);
  if (values->StartBorder().IsVisible()) {
    start_.value = &values->StartBorder();
    start_.inner_width = cell_.CollapsedInnerBorderStart();
    start_.outer_width = cell_.CollapsedOuterBorderStart();
  } else {
    start_.value = nullptr;
  }

  if (values->EndBorder().IsVisible()) {
    end_.value = &values->EndBorder();
    end_.inner_width = cell_.CollapsedInnerBorderEnd();
    end_.outer_width = cell_.CollapsedOuterBorderEnd();
  } else {
    end_.value = nullptr;
  }

  if (values->BeforeBorder().IsVisible()) {
    before_.value = &values->BeforeBorder();
    before_.inner_width = cell_.CollapsedInnerBorderBefore();
    before_.outer_width = cell_.CollapsedOuterBorderBefore();
  } else {
    before_.value = nullptr;
  }

  if (values->AfterBorder().IsVisible()) {
    after_.value = &values->AfterBorder();
    after_.inner_width = cell_.CollapsedInnerBorderAfter();
    after_.outer_width = cell_.CollapsedOuterBorderAfter();
  } else {
    after_.value = nullptr;
  }

  // At first, let all borders paint the corners. This is to keep the current
  // behavior for layout tests e.g. css2.1/t170602-bdr-conflict-w-01-d.html.
  // TODO(crbug.com/672216): Determine the best way to deal with this.
  if (start_.value && before_.value) {
    start_.begin_outset = before_.outer_width;
    before_.begin_outset = start_.outer_width;
  }
  if (end_.value && before_.value) {
    end_.begin_outset = before_.outer_width;
    before_.end_outset = end_.outer_width;
  }
  if (start_.value && after_.value) {
    start_.end_outset = after_.outer_width;
    after_.begin_outset = start_.outer_width;
  }
  if (after_.value && end_.value) {
    end_.end_outset = after_.outer_width;
    after_.end_outset = end_.outer_width;
  }

  // Skip painting the start border if it will be painted by the preceding cell
  // as its end border.
  if (start_.value && SameBefore(cell_preceding_) &&
      cell_preceding_->RowSpan() >= cell_.RowSpan()) {
    start_.value = nullptr;
    // Otherwise we'll still paint the shared border twice which may cause
    // incorrect border conflict resolution for row/col spanning cells.
    // TODO(crbug.com/2902 etc.): Paint collapsed borders by grid cells.
  }
  // Skip painting the before border if it will be painted by the above cell
  // as its after border.
  if (before_.value && SameStart(cell_above_) &&
      cell_above_->ColSpan() >= cell_.ColSpan()) {
    before_.value = nullptr;
    // Otherwise we'll still paint the shared border twice which may cause
    // incorrect border conflict resolution for row/col spanning cells.
    // TODO(crbug.com/2902 etc.): Paint collapsed borders by grid cells.
  }
}

static const LayoutTableCell::CollapsedBorderValues* GetCollapsedBorderValues(
    const LayoutTableCell* cell) {
  return cell ? cell->GetCollapsedBorderValues() : nullptr;
}

void CollapsedBorderPainter::AdjustCorners() {
  const auto* borders_preceding = GetCollapsedBorderValues(cell_preceding_);
  const auto* borders_above = GetCollapsedBorderValues(cell_above_);
  const auto* borders_following = GetCollapsedBorderValues(cell_following_);
  const auto* borders_below = GetCollapsedBorderValues(cell_below_);

  // These variables indicate whether |cell_| shares corners with the adjacent
  // cells, e.g. if before_start_adjoins_preceding is true, |cell_| shares the
  // before-start (logical top-left) corner with |cell_preceding_|.
  bool before_start_adjoins_preceding =
      borders_preceding && SameBefore(cell_preceding_);
  bool before_start_adjoins_above = borders_above && SameStart(cell_above_);
  bool before_end_adjoins_above = borders_above && SameEnd(cell_above_);
  bool before_end_adjoins_following =
      borders_following && SameBefore(cell_following_);
  bool after_end_adjoins_following =
      borders_following && SameAfter(cell_following_);
  bool after_end_adjoins_below = borders_below && SameEnd(cell_below_);
  bool after_start_adjoins_below = borders_below && SameStart(cell_below_);
  bool after_start_adjoins_preceding =
      borders_preceding && SameAfter(cell_preceding_);

  if (start_.value) {
    if (start_.value->CoversCorner(
            before_.value,
            before_start_adjoins_preceding ? &borders_preceding->BeforeBorder()
                                           : nullptr,
            before_start_adjoins_above ? &borders_above->StartBorder()
                                       : nullptr)) {
      if (before_start_adjoins_preceding) {
        start_.begin_outset = std::max<int>(
            start_.begin_outset, cell_preceding_->CollapsedOuterBorderBefore());
      }
    } else {
      start_.begin_outset =
          -std::max(cell_.CollapsedInnerBorderBefore(),
                    before_start_adjoins_preceding
                        ? cell_preceding_->CollapsedInnerBorderBefore()
                        : 0);
    }
    if (start_.value->CoversCorner(
            after_.value,
            after_start_adjoins_preceding ? &borders_preceding->AfterBorder()
                                          : nullptr,
            after_start_adjoins_below ? &borders_below->StartBorder()
                                      : nullptr)) {
      if (after_start_adjoins_preceding) {
        start_.end_outset = std::max<int>(
            start_.end_outset, cell_preceding_->CollapsedOuterBorderAfter());
      }
    } else {
      start_.end_outset =
          -std::max(cell_.CollapsedInnerBorderAfter(),
                    after_start_adjoins_preceding
                        ? cell_preceding_->CollapsedInnerBorderAfter()
                        : 0);
    }
  }

  if (end_.value) {
    if (end_.value->CoversCorner(
            before_.value,
            before_end_adjoins_following ? &borders_following->BeforeBorder()
                                         : nullptr,
            before_end_adjoins_above ? &borders_above->EndBorder() : nullptr)) {
      if (before_end_adjoins_following) {
        end_.begin_outset = std::max<int>(
            end_.begin_outset, cell_following_->CollapsedOuterBorderBefore());
      }
    } else {
      end_.begin_outset =
          -std::max(cell_.CollapsedInnerBorderBefore(),
                    before_end_adjoins_following
                        ? cell_following_->CollapsedInnerBorderBefore()
                        : 0);
    }
    if (end_.value->CoversCorner(
            after_.value,
            after_end_adjoins_following ? &borders_following->AfterBorder()
                                        : nullptr,
            after_end_adjoins_below ? &borders_below->EndBorder() : nullptr)) {
      if (after_end_adjoins_following) {
        end_.end_outset = std::max<int>(
            end_.end_outset, cell_following_->CollapsedOuterBorderAfter());
      }
    } else {
      end_.end_outset =
          -std::max(cell_.CollapsedInnerBorderAfter(),
                    after_end_adjoins_following
                        ? cell_following_->CollapsedInnerBorderAfter()
                        : 0);
    }
  }

  if (before_.value) {
    if (before_.value->CoversCorner(
            start_.value,
            before_start_adjoins_preceding ? &borders_preceding->BeforeBorder()
                                           : nullptr,
            before_start_adjoins_above ? &borders_above->StartBorder()
                                       : nullptr)) {
      if (before_start_adjoins_above) {
        before_.begin_outset = std::max<int>(
            before_.begin_outset, cell_above_->CollapsedOuterBorderStart());
      }
    } else {
      before_.begin_outset = -std::max(
          cell_.CollapsedInnerBorderStart(),
          before_start_adjoins_above ? cell_above_->CollapsedInnerBorderStart()
                                     : 0);
    }
    if (before_.value->CoversCorner(
            end_.value,
            before_end_adjoins_following ? &borders_following->BeforeBorder()
                                         : nullptr,
            before_end_adjoins_above ? &borders_above->EndBorder() : nullptr)) {
      if (before_end_adjoins_above) {
        before_.end_outset = std::max<int>(
            before_.end_outset, cell_above_->CollapsedOuterBorderEnd());
      }
    } else {
      before_.end_outset = -std::max(
          cell_.CollapsedInnerBorderEnd(),
          before_end_adjoins_above ? cell_above_->CollapsedInnerBorderEnd()
                                   : 0);
    }
  }

  if (after_.value) {
    if (after_.value->CoversCorner(
            start_.value,
            after_start_adjoins_preceding ? &borders_preceding->AfterBorder()
                                          : nullptr,
            after_start_adjoins_below ? &borders_below->StartBorder()
                                      : nullptr)) {
      if (after_start_adjoins_below) {
        after_.begin_outset = std::max<int>(
            after_.begin_outset, cell_below_->CollapsedOuterBorderStart());
      }
    } else {
      after_.begin_outset = -std::max(
          cell_.CollapsedInnerBorderStart(),
          after_start_adjoins_below ? cell_below_->CollapsedInnerBorderStart()
                                    : 0);
    }
    if (after_.value->CoversCorner(
            end_.value,
            after_end_adjoins_following ? &borders_following->AfterBorder()
                                        : nullptr,
            after_end_adjoins_below ? &borders_below->EndBorder() : nullptr)) {
      if (after_end_adjoins_below) {
        after_.end_outset = std::max<int>(
            after_.end_outset, cell_below_->CollapsedOuterBorderEnd());
      }
    } else {
      after_.end_outset = -std::max(
          cell_.CollapsedInnerBorderEnd(),
          after_end_adjoins_below ? cell_below_->CollapsedInnerBorderEnd() : 0);
    }
  }
}

void CollapsedBorderPainter::AdjustForWritingModeAndDirection() {
  const auto& style = cell_.StyleForCellFlow();
  if (!style.IsLeftToRightDirection()) {
    std::swap(start_, end_);
    std::swap(before_.begin_outset, before_.end_outset);
    std::swap(after_.begin_outset, after_.end_outset);
  }
  if (!style.IsHorizontalWritingMode()) {
    std::swap(after_, end_);
    std::swap(before_, start_);
    if (style.IsFlippedBlocksWritingMode()) {
      std::swap(start_, end_);
      std::swap(before_.begin_outset, before_.end_outset);
      std::swap(after_.begin_outset, after_.end_outset);
    }
  }
}

static EBorderStyle CollapsedBorderStyle(EBorderStyle style) {
  if (style == EBorderStyle::kOutset)
    return EBorderStyle::kGroove;
  if (style == EBorderStyle::kInset)
    return EBorderStyle::kRidge;
  return style;
}

const DisplayItemClient& CollapsedBorderPainter::DisplayItemClientForBorders()
    const {
  // TODO(wkorman): We may need to handle PaintInvalidationDelayedFull.
  // http://crbug.com/657186
  return cell_.UsesCompositedCellDisplayItemClients()
             ? static_cast<const DisplayItemClient&>(
                   *cell_.GetCollapsedBorderValues())
             : cell_;
}

void CollapsedBorderPainter::PaintCollapsedBorders(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset) {
  if (cell_.Style()->Visibility() != EVisibility::kVisible)
    return;

  if (!cell_.GetCollapsedBorderValues())
    return;

  LayoutPoint adjusted_paint_offset = paint_offset + cell_.Location();
  if (!BlockPainter(cell_).IntersectsPaintRect(paint_info,
                                               adjusted_paint_offset))
    return;

  GraphicsContext& context = paint_info.context;
  const DisplayItemClient& client = DisplayItemClientForBorders();
  if (DrawingRecorder::UseCachedDrawingIfPossible(
          context, client, DisplayItem::kTableCellCollapsedBorders))
    return;

  cell_preceding_ = table_.CellPreceding(cell_);
  cell_following_ = table_.CellFollowing(cell_);
  cell_above_ = table_.CellAbove(cell_);
  cell_below_ = table_.CellBelow(cell_);

  SetupBorders();
  AdjustCorners();
  AdjustForWritingModeAndDirection();
  // Now left=start_, right=end_, before_=top, after_=bottom.

  // Collapsed borders are half inside and half outside of |rect|.
  IntRect rect = PixelSnappedIntRect(adjusted_paint_offset, cell_.Size());
  // |paint_rect| covers the whole collapsed borders.
  IntRect paint_rect = rect;
  paint_rect.Expand(IntRectOutsets(before_.outer_width, end_.outer_width,
                                   after_.outer_width, start_.outer_width));
  DrawingRecorder recorder(context, client,
                           DisplayItem::kTableCellCollapsedBorders, paint_rect);

  // We never paint diagonals at the joins.  We simply let the border with the
  // highest precedence paint on top of borders with lower precedence.
  if (before_.value) {
    ObjectPainter::DrawLineForBoxSide(
        context, rect.X() - before_.begin_outset,
        rect.Y() - before_.outer_width, rect.MaxX() + before_.end_outset,
        rect.Y() + before_.inner_width, kBSTop, before_.value->GetColor(),
        CollapsedBorderStyle(before_.value->Style()), 0, 0, true);
  }
  if (after_.value) {
    ObjectPainter::DrawLineForBoxSide(
        context, rect.X() - after_.begin_outset,
        rect.MaxY() - after_.inner_width, rect.MaxX() + after_.end_outset,
        rect.MaxY() + after_.outer_width, kBSBottom, after_.value->GetColor(),
        CollapsedBorderStyle(after_.value->Style()), 0, 0, true);
  }
  if (start_.value) {
    ObjectPainter::DrawLineForBoxSide(
        context, rect.X() - start_.outer_width, rect.Y() - start_.begin_outset,
        rect.X() + start_.inner_width, rect.MaxY() + start_.end_outset, kBSLeft,
        start_.value->GetColor(), CollapsedBorderStyle(start_.value->Style()),
        0, 0, true);
  }
  if (end_.value) {
    ObjectPainter::DrawLineForBoxSide(
        context, rect.MaxX() - end_.inner_width, rect.Y() - end_.begin_outset,
        rect.MaxX() + end_.outer_width, rect.MaxY() + end_.end_outset, kBSRight,
        end_.value->GetColor(), CollapsedBorderStyle(end_.value->Style()), 0, 0,
        true);
  }
}

}  // namespace blink
