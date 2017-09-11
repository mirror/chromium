// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/BaselineAlignment.h"

#include "core/layout/LayoutGrid.h"
#include "core/style/ComputedStyle.h"

namespace blink {

LayoutUnit BaselineAlignment::LogicalAscentForChild(
    const LayoutBox& child,
    GridAxis baseline_axis) const {
  LayoutUnit ascent = AscentForChild(child, baseline_axis);
  return IsDescentBaselineForChild(child, baseline_axis)
             ? DescentForChild(child, ascent, baseline_axis)
             : ascent;
}

LayoutUnit BaselineAlignment::AscentForChild(const LayoutBox& child,
                                             GridAxis baseline_axis) const {
  const LayoutGrid& layout_grid = track_sizing_algorithm_.GetLayoutGrid();
  LayoutUnit margin = IsDescentBaselineForChild(child, baseline_axis)
                          ? GridLayoutUtils::MarginUnderForChild(
                                layout_grid, child, baseline_axis)
                          : GridLayoutUtils::MarginOverForChild(
                                layout_grid, child, baseline_axis);
  LayoutUnit baseline = GridLayoutUtils::IsParallelToBlockAxisForChild(
                            layout_grid, child, baseline_axis)
                            ? child.FirstLineBoxBaseline()
                            : LayoutUnit(-1);
  // We take border-box's under edge if no valid baseline.
  if (baseline == -1) {
    if (GridLayoutUtils::IsHorizontalGridAxis(layout_grid, baseline_axis)) {
      return layout_grid.StyleRef().IsFlippedBlocksWritingMode()
                 ? child.Size().Width().ToInt() + margin
                 : margin;
    }
    return child.Size().Height() + margin;
  }
  return baseline + margin;
}

LayoutUnit BaselineAlignment::DescentForChild(const LayoutBox& child,
                                              LayoutUnit ascent,
                                              GridAxis baseline_axis) const {
  const LayoutGrid& layout_grid = track_sizing_algorithm_.GetLayoutGrid();
  if (GridLayoutUtils::IsParallelToBlockAxisForChild(layout_grid, child,
                                                     baseline_axis))
    return child.MarginLogicalHeight() + child.LogicalHeight() - ascent;
  return child.MarginLogicalWidth() + child.LogicalWidth() - ascent;
}

bool BaselineAlignment::IsDescentBaselineForChild(
    const LayoutBox& child,
    GridAxis baseline_axis) const {
  const LayoutGrid& layout_grid = track_sizing_algorithm_.GetLayoutGrid();
  return GridLayoutUtils::IsHorizontalGridAxis(layout_grid, baseline_axis) &&
         ((child.StyleRef().IsFlippedBlocksWritingMode() &&
           !layout_grid.StyleRef().IsFlippedBlocksWritingMode()) ||
          (child.StyleRef().IsFlippedLinesWritingMode() &&
           layout_grid.StyleRef().IsFlippedBlocksWritingMode()));
}

bool BaselineAlignment::IsBaselineContextComputed(
    GridAxis baseline_axis) const {
  return baseline_axis == kGridColumnAxis
             ? !row_axis_alignment_context_.IsEmpty()
             : !col_axis_alignment_context_.IsEmpty();
}

const BaselineGroup& BaselineAlignment::GetBaselineGroupForChild(
    const LayoutBox& child,
    GridAxis baseline_axis) const {
  const LayoutGrid& layout_grid = track_sizing_algorithm_.GetLayoutGrid();
  DCHECK(layout_grid.IsBaselineAlignmentForChild(child, baseline_axis));
  bool is_column_axis_baseline = baseline_axis == kGridColumnAxis;
  bool is_row_axis_context = is_column_axis_baseline;
  const Grid& grid = track_sizing_algorithm_.GetGrid();
  const auto& span = is_row_axis_context
                         ? grid.GridItemSpan(child, kForRows)
                         : grid.GridItemSpan(child, kForColumns);
  auto& contexts_map = is_row_axis_context ? row_axis_alignment_context_
                                           : col_axis_alignment_context_;
  auto* context = contexts_map.at(span.StartLine());
  DCHECK(context);
  ItemPosition align =
      layout_grid.SelfAlignmentForChild(baseline_axis, child).GetPosition();
  return context->GetSharedGroup(child, align);
}

void BaselineAlignment::UpdateBaselineAlignmentContextIfNeeded(
    LayoutBox& child,
    GridAxis baseline_axis) {
  // TODO (lajava): We must ensure this method is not called as part of an
  // intrinsic size computation.
  const LayoutGrid& layout_grid = track_sizing_algorithm_.GetLayoutGrid();
  if (!layout_grid.IsBaselineAlignmentForChild(child, baseline_axis))
    return;

  child.LayoutIfNeeded();

  // Determine Ascent and Descent values of this child with respect to
  // its grid container.
  LayoutUnit ascent = AscentForChild(child, baseline_axis);
  LayoutUnit descent = DescentForChild(child, ascent, baseline_axis);
  if (IsDescentBaselineForChild(child, baseline_axis))
    std::swap(ascent, descent);

  // Looking up for a shared alignment context perpendicular to the
  // baseline axis.
  bool is_column_axis_baseline = baseline_axis == kGridColumnAxis;
  bool is_row_axis_context = is_column_axis_baseline;
  const Grid& grid = track_sizing_algorithm_.GetGrid();
  const auto& span = is_row_axis_context
                         ? grid.GridItemSpan(child, kForRows)
                         : grid.GridItemSpan(child, kForColumns);
  auto& contexts_map = is_row_axis_context ? row_axis_alignment_context_
                                           : col_axis_alignment_context_;
  auto add_result = contexts_map.insert(span.StartLine(), nullptr);

  // Looking for a compatible baseline-sharing group.
  ItemPosition align =
      layout_grid.SelfAlignmentForChild(baseline_axis, child).GetPosition();
  if (add_result.is_new_entry) {
    add_result.stored_value->value =
        WTF::MakeUnique<BaselineContext>(child, align, ascent, descent);
  } else {
    auto* context = add_result.stored_value->value.get();
    context->UpdateSharedGroup(child, align, ascent, descent);
  }
}

LayoutUnit BaselineAlignment::BaselineOffsetForChild(
    const LayoutBox& child,
    GridAxis baseline_axis) const {
  const LayoutGrid& layout_grid = track_sizing_algorithm_.GetLayoutGrid();
  if (!layout_grid.IsBaselineAlignmentForChild(child, baseline_axis))
    return LayoutUnit();
  auto& group = GetBaselineGroupForChild(child, baseline_axis);
  if (group.size() > 1)
    return group.MaxAscent() - LogicalAscentForChild(child, baseline_axis);
  return LayoutUnit();
}

Optional<LayoutUnit> BaselineAlignment::ExtentForBaselineAlignment(
    const LayoutBox& child) const {
  const LayoutGrid& layout_grid = track_sizing_algorithm_.GetLayoutGrid();
  GridAxis baseline_axis =
      GridLayoutUtils::IsOrthogonalChild(layout_grid, child) ? kGridRowAxis
                                                             : kGridColumnAxis;
  if (!layout_grid.IsBaselineAlignmentForChild(child, baseline_axis) ||
      !IsBaselineContextComputed(baseline_axis))
    return WTF::nullopt;

  auto& group = GetBaselineGroupForChild(child, baseline_axis);
  return group.MaxAscent() + group.MaxDescent();
}

bool BaselineAlignment::BaselineMayAffectIntrinsicSize(
    GridTrackSizingDirection direction) const {
  const auto& contexts_map = direction == kForColumns
                                 ? col_axis_alignment_context_
                                 : row_axis_alignment_context_;
  for (const auto& context : contexts_map) {
    auto track_size =
        track_sizing_algorithm_.GetGridTrackSize(direction, context.key);
    // TODO(lajava): Should we consider flexible tracks as well ?
    if (!track_size.IsContentSized())
      continue;
    for (const auto& group : context.value->SharedGroups()) {
      if (group.size() > 1) {
        auto grid_area_size =
            track_sizing_algorithm_.Tracks(direction)[context.key].BaseSize();
        if (group.MaxAscent() + group.MaxDescent() > grid_area_size)
          return true;
      }
    }
  }
  return false;
}

void BaselineAlignment::Clear() {
  row_axis_alignment_context_.clear();
  col_axis_alignment_context_.clear();
}

BaselineGroup::BaselineGroup(WritingMode block_flow,
                             ItemPosition child_preference)
    : max_ascent_(0), max_descent_(0), items_() {
  block_flow_ = block_flow;
  preference_ = child_preference;
}

void BaselineGroup::Update(const LayoutBox& child,
                           LayoutUnit ascent,
                           LayoutUnit descent) {
  if (items_.insert(&child).is_new_entry) {
    max_ascent_ = std::max(max_ascent_, ascent);
    max_descent_ = std::max(max_descent_, descent);
  }
}

bool BaselineGroup::IsOppositeBlockFlow(WritingMode block_flow) const {
  switch (block_flow) {
    case WritingMode::kHorizontalTb:
      return false;
    case WritingMode::kVerticalLr:
      return block_flow_ == WritingMode::kVerticalRl;
    case WritingMode::kVerticalRl:
      return block_flow_ == WritingMode::kVerticalLr;
    default:
      NOTREACHED();
      return false;
  }
}

bool BaselineGroup::IsOrthogonalBlockFlow(WritingMode block_flow) const {
  switch (block_flow) {
    case WritingMode::kHorizontalTb:
      return block_flow_ != WritingMode::kHorizontalTb;
    case WritingMode::kVerticalLr:
    case WritingMode::kVerticalRl:
      return block_flow_ == WritingMode::kHorizontalTb;
    default:
      NOTREACHED();
      return false;
  }
}

bool BaselineGroup::IsCompatible(WritingMode child_block_flow,
                                 ItemPosition child_preference) const {
  DCHECK(IsBaselinePosition(child_preference));
  DCHECK_GT(size(), 0);
  return ((block_flow_ == child_block_flow ||
           IsOrthogonalBlockFlow(child_block_flow)) &&
          preference_ == child_preference) ||
         (IsOppositeBlockFlow(child_block_flow) &&
          preference_ != child_preference);
}

BaselineContext::BaselineContext(const LayoutBox& child,
                                 ItemPosition preference,
                                 LayoutUnit ascent,
                                 LayoutUnit descent) {
  DCHECK(IsBaselinePosition(preference));
  UpdateSharedGroup(child, preference, ascent, descent);
}

const BaselineGroup& BaselineContext::GetSharedGroup(
    const LayoutBox& child,
    ItemPosition preference) const {
  DCHECK(IsBaselinePosition(preference));
  return const_cast<BaselineContext*>(this)->FindCompatibleSharedGroup(
      child, preference);
}

void BaselineContext::UpdateSharedGroup(const LayoutBox& child,
                                        ItemPosition preference,
                                        LayoutUnit ascent,
                                        LayoutUnit descent) {
  DCHECK(IsBaselinePosition(preference));
  BaselineGroup& group = FindCompatibleSharedGroup(child, preference);
  group.Update(child, ascent, descent);
}

// TODO Properly implement baseline-group compatibility
// See https://github.com/w3c/csswg-drafts/issues/721
BaselineGroup& BaselineContext::FindCompatibleSharedGroup(
    const LayoutBox& child,
    ItemPosition preference) {
  WritingMode block_direction = child.StyleRef().GetWritingMode();
  for (auto& group : shared_groups_) {
    if (group.IsCompatible(block_direction, preference))
      return group;
  }
  shared_groups_.push_front(BaselineGroup(block_direction, preference));
  return shared_groups_[0];
}

}  // namespace blink
