// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/ng_column_layout_algorithm.h"

#include "core/layout/ng/inline/ng_baseline.h"
#include "core/layout/ng/ng_block_layout_algorithm.h"
#include "core/layout/ng/ng_constraint_space_builder.h"
#include "core/layout/ng/ng_length_utils.h"
#include "core/layout/ng/ng_out_of_flow_layout_part.h"

namespace blink {

NGColumnLayoutAlgorithm::NGColumnLayoutAlgorithm(NGBlockNode node,
                                                 NGConstraintSpace* space,
                                                 NGBreakToken* break_token)
    : NGLayoutAlgorithm(node, space, ToNGBlockBreakToken(break_token)) {}

RefPtr<NGLayoutResult> NGColumnLayoutAlgorithm::Layout() {
  Optional<MinMaxSize> min_max_size;
  if (NeedMinMaxSize(ConstraintSpace(), Style()))
    min_max_size = ComputeMinMaxSize();
  NGBoxStrut border_scrollbar_padding = CalculateBorderScrollbarPadding(
      ConstraintSpace(), Style(), Node().GetLayoutObject());
  NGLogicalSize border_box_size =
      CalculateBorderBoxSize(ConstraintSpace(), Style(), min_max_size);
  content_box_size_ =
      CalculateContentBoxSize(border_box_size, border_scrollbar_padding);

  // TODO(mstensho): Implement column balancing.
  DCHECK_NE(Style().GetColumnFill(), EColumnFill::kBalance)
      << "Column balancing is not yet implemented";
  // Auto height also implies column balancing.
  DCHECK_NE(border_box_size.block_size, NGSizeIndefinite)
      << "Column balancing is not yet implemented";

  container_builder_.SetSize(border_box_size);

  RefPtr<NGConstraintSpace> child_space = CreateConstraintSpaceForColumns();
  NGWritingMode writing_mode = ConstraintSpace().WritingMode();
  RefPtr<NGBlockBreakToken> break_token = BreakToken();
  NGLogicalSize overflow;
  LayoutUnit column_inline_offset;
  LayoutUnit column_inline_progression =
      child_space->AvailableSize().inline_size + ResolveUsedColumnGap(Style());

  do {
    // Lay out one column. Each column will become a fragment.
    NGBlockLayoutAlgorithm child_algorithm(Node(), child_space.Get(),
                                           break_token.Get());
    RefPtr<NGLayoutResult> result = child_algorithm.Layout();
    RefPtr<NGPhysicalBoxFragment> column(
        ToNGPhysicalBoxFragment(result->PhysicalFragment().Get()));

    NGLogicalOffset logical_offset(column_inline_offset, LayoutUnit());
    container_builder_.AddChild(column, logical_offset);

    NGLogicalSize size = NGBoxFragment(writing_mode, column.Get()).Size();
    NGLogicalOffset end_offset =
        logical_offset + NGLogicalOffset(size.inline_size, size.block_size);

    overflow.inline_size =
        std::max(overflow.inline_size, end_offset.inline_offset);
    overflow.block_size =
        std::max(overflow.block_size, end_offset.block_offset);

    column_inline_offset += column_inline_progression;
    break_token = ToNGBlockBreakToken(column->BreakToken());
  } while (break_token && !break_token->IsFinished());

  container_builder_.SetOverflowSize(overflow);

  NGOutOfFlowLayoutPart(ConstraintSpace(), Style(), &container_builder_).Run();

  // TODO(mstensho): Propagate baselines.

  return container_builder_.ToBoxFragment();
}

RefPtr<NGConstraintSpace>
NGColumnLayoutAlgorithm::CreateConstraintSpaceForColumns() const {
  NGConstraintSpaceBuilder space_builder(ConstraintSpace().WritingMode());
  NGLogicalSize adjusted_size = content_box_size_;
  adjusted_size.inline_size =
      ResolveUsedColumnInlineSize(adjusted_size.inline_size, Style());
  space_builder.SetAvailableSize(adjusted_size);
  space_builder.SetPercentageResolutionSize(adjusted_size);

  if (NGBaseline::ShouldPropagateBaselines(Node()))
    space_builder.AddBaselineRequests(ConstraintSpace().BaselineRequests());

  space_builder.SetFragmentationType(kFragmentColumn);
  space_builder.SetFragmentainerSpaceAvailable(adjusted_size.block_size);
  space_builder.SetIsNewFormattingContext(true);
  space_builder.SetIsAnonymous(true);

  return space_builder.ToConstraintSpace(
      FromPlatformWritingMode(Style().GetWritingMode()));
}

}  // namespace Blink
