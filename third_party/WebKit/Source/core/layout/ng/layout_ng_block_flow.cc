// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/layout_ng_block_flow.h"

#include "core/layout/LayoutAnalyzer.h"
#include "core/layout/ng/ng_constraint_space.h"
#include "core/layout/ng/ng_fragment.h"
#include "core/layout/ng/ng_layout_result.h"

namespace blink {

LayoutNGBlockFlow::LayoutNGBlockFlow(Element* element)
    : LayoutBlockFlow(element) {}

bool LayoutNGBlockFlow::IsOfType(LayoutObjectType type) const {
  return type == kLayoutObjectNGBlockFlow || LayoutBlockFlow::IsOfType(type);
}

void LayoutNGBlockFlow::UpdateBlockLayout(bool relayout_children) {
  LayoutAnalyzer::BlockScope analyzer(*this);

  Optional<LayoutUnit> override_logical_width;
  Optional<LayoutUnit> override_logical_height;

  if (IsOutOfFlowPositioned()) {
    // LegacyLayout and LayoutNG use different strategies to set size of
    // an OOF positioned child. In Legacy lets child computes the size,
    // in NG size is "forced" from parent to child.
    // This is a compat layer that "forces" size computed by Legacy
    // on an NG child.
    LogicalExtentComputedValues computed_values;
    const ComputedStyle* style = Style();
    bool logical_width_is_shrink_to_fit =
        style->LogicalWidth().IsAuto() &&
        (style->LogicalLeft().IsAuto() || style->LogicalRight().IsAuto());
    if (!logical_width_is_shrink_to_fit) {
      ComputeLogicalWidth(computed_values);
      override_logical_width =
          computed_values.extent_ - BorderAndPaddingLogicalWidth();
    }
    if (!StyleRef().LogicalHeight().IsAuto()) {
      ComputeLogicalHeight(computed_values);
      override_logical_height =
          computed_values.extent_ - BorderAndPaddingLogicalHeight();
    }
  }

  RefPtr<NGConstraintSpace> constraint_space =
      NGConstraintSpace::CreateFromLayoutObject(*this, override_logical_width,
                                                override_logical_height);

  RefPtr<NGLayoutResult> result =
      NGBlockNode(this).Layout(constraint_space.Get());

  if (IsOutOfFlowPositioned()) {
    // In legacy layout, abspos differs from regular blocks in that abspos
    // blocks position themselves in their own layout, instead of getting
    // positioned by their parent. So it we are a positioned block in a legacy-
    // layout containing block, we have to emulate this positioning.
    // Additionally, until we natively support abspos in LayoutNG, this code
    // will also be reached though the layoutPositionedObjects call in
    // NGBlockNode::CopyFragmentDataToLayoutBox.
    LogicalExtentComputedValues computed_values;
    ComputeLogicalWidth(computed_values);
    SetLogicalLeft(computed_values.position_);
    ComputeLogicalHeight(LogicalHeight(), LogicalTop(), computed_values);
    SetLogicalTop(computed_values.position_);
  }

  for (NGOutOfFlowPositionedDescendant descendant :
       result->OutOfFlowPositionedDescendants())
    descendant.node.UseOldOutOfFlowPositioning();
}

NGInlineNodeData& LayoutNGBlockFlow::GetNGInlineNodeData() const {
  DCHECK(ng_inline_node_data_);
  return *ng_inline_node_data_.get();
}

void LayoutNGBlockFlow::ResetNGInlineNodeData() {
  ng_inline_node_data_ = WTF::MakeUnique<NGInlineNodeData>();
}

int LayoutNGBlockFlow::FirstLineBoxBaseline() const {
  // TODO(kojii): Implement. This will stop working once we stop creating line
  // boxes.
  return LayoutBlockFlow::FirstLineBoxBaseline();
}

int LayoutNGBlockFlow::InlineBlockBaseline(
    LineDirectionMode line_direction) const {
  // TODO(kojii): Implement. This will stop working once we stop creating line
  // boxes.
  return LayoutBlockFlow::InlineBlockBaseline(line_direction);
}

}  // namespace blink
