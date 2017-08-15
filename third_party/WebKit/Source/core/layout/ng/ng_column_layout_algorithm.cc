// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/ng_column_layout_algorithm.h"

#include "core/layout/ng/inline/ng_baseline.h"
#include "core/layout/ng/ng_constraint_space_builder.h"
#include "core/layout/ng/ng_length_utils.h"

namespace blink {

NGColumnLayoutAlgorithm::NGColumnLayoutAlgorithm(NGBlockNode node,
                                                 NGConstraintSpace* space,
                                                 NGBreakToken* break_token)
    : NGBlockLayoutAlgorithm(node, space, ToNGBlockBreakToken(break_token)){};

RefPtr<NGLayoutResult> NGColumnLayoutAlgorithm::Layout() {
  PrepareLayout();

  RefPtr<NGConstraintSpace> child_space = CreateConstraintSpaceForColumns();
  NGWritingMode writing_mode = ConstraintSpace().WritingMode();
  RefPtr<NGBlockBreakToken> break_token = BreakToken();
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
    PropagateSizeFromChild(end_offset);

    column_inline_offset += column_inline_progression;
    break_token = ToNGBlockBreakToken(column->BreakToken());
  } while (break_token && !break_token->IsFinished());

  UpdateSizeAfterChildLayout();

  return FinishLayout();
}

RefPtr<NGConstraintSpace>
NGColumnLayoutAlgorithm::CreateConstraintSpaceForColumns() const {
  NGConstraintSpaceBuilder space_builder(&ConstraintSpace());
  NGLogicalSize adjusted_size = ChildAvailableSize();
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
