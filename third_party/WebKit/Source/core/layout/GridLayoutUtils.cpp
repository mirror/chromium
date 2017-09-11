// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/GridLayoutUtils.h"

#include "core/layout/LayoutGrid.h"
#include "platform/LengthFunctions.h"

namespace blink {

static LayoutUnit ComputeMarginLogicalSizeForChild(
    const LayoutGrid& grid,
    MarginDirection for_direction,
    const LayoutBox& child) {
  if (!child.StyleRef().HasMargin())
    return LayoutUnit();

  bool is_inline_direction = for_direction == kInlineDirection;
  LayoutUnit margin_start;
  LayoutUnit margin_end;
  LayoutUnit logical_size =
      is_inline_direction ? child.LogicalWidth() : child.LogicalHeight();
  Length margin_start_length = is_inline_direction
                                   ? child.StyleRef().MarginStart()
                                   : child.StyleRef().MarginBefore();
  Length margin_end_length = is_inline_direction
                                 ? child.StyleRef().MarginEnd()
                                 : child.StyleRef().MarginAfter();
  child.ComputeMarginsForDirection(
      for_direction, &grid, child.ContainingBlockLogicalWidthForContent(),
      logical_size, margin_start, margin_end, margin_start_length,
      margin_end_length);

  return margin_start + margin_end;
}

LayoutUnit GridLayoutUtils::MarginOverForChild(const LayoutGrid& grid,
                                               const LayoutBox& child,
                                               GridAxis axis) {
  return IsHorizontalGridAxis(grid, axis) ? child.MarginRight()
                                          : child.MarginTop();
}

LayoutUnit GridLayoutUtils::MarginUnderForChild(const LayoutGrid& grid,
                                                const LayoutBox& child,
                                                GridAxis axis) {
  return IsHorizontalGridAxis(grid, axis) ? child.MarginLeft()
                                          : child.MarginBottom();
}

LayoutUnit GridLayoutUtils::MarginLogicalWidthForChild(const LayoutGrid& grid,
                                                       const LayoutBox& child) {
  return child.NeedsLayout()
             ? ComputeMarginLogicalSizeForChild(grid, kInlineDirection, child)
             : child.MarginLogicalWidth();
}

LayoutUnit GridLayoutUtils::MarginLogicalHeightForChild(
    const LayoutGrid& grid,
    const LayoutBox& child) {
  return child.NeedsLayout()
             ? ComputeMarginLogicalSizeForChild(grid, kBlockDirection, child)
             : child.MarginLogicalHeight();
}

bool GridLayoutUtils::IsHorizontalGridAxis(const LayoutGrid& grid,
                                           GridAxis axis) {
  return axis == kGridRowAxis ? grid.IsHorizontalWritingMode()
                              : !grid.IsHorizontalWritingMode();
}

bool GridLayoutUtils::IsOrthogonalChild(const LayoutGrid& grid,
                                        const LayoutBox& child) {
  return child.IsHorizontalWritingMode() != grid.IsHorizontalWritingMode();
}

GridTrackSizingDirection GridLayoutUtils::FlowAwareDirectionForChild(
    const LayoutGrid& grid,
    const LayoutBox& child,
    GridTrackSizingDirection direction) {
  return !IsOrthogonalChild(grid, child)
             ? direction
             : (direction == kForColumns ? kForRows : kForColumns);
}

bool GridLayoutUtils::IsParallelToBlockAxisForChild(const LayoutGrid& grid,
                                                    const LayoutBox& child,
                                                    GridAxis axis) {
  return axis == kGridColumnAxis ? !IsOrthogonalChild(grid, child)
                                 : IsOrthogonalChild(grid, child);
}

}  // namespace blink
