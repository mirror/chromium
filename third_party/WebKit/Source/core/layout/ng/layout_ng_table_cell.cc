// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/layout_ng_block_flow.h"

#include "core/layout/LayoutAnalyzer.h"
#include "core/layout/ng/inline/ng_inline_node_data.h"
#include "core/layout/ng/ng_constraint_space.h"
#include "core/layout/ng/ng_layout_result.h"
#include "core/layout/ng/ng_length_utils.h"
#include "core/paint/ng/ng_block_flow_painter.h"
#include "platform/RuntimeEnabledFeatures.h"

namespace blink {

LayoutNGTableCell::LayoutNGTableCell(Element* element)
    : LayoutTableCell(element) {}

LayoutNGTableCell::~LayoutNGTableCell() {}

bool LayoutNGTableCell::IsOfType(LayoutObjectType type) const {
  return type == kLayoutObjectNGTableCell || LayoutTableCell::IsOfType(type);
}

void LayoutNGTableCell::UpdateBlockLayout(bool relayout_children) {
  LayoutAnalyzer::BlockScope analyzer(*this);

  Optional<LayoutUnit> override_logical_width;
  Optional<LayoutUnit> override_logical_height;

  override_logical_width = LogicalWidth() - BorderAndPaddingLogicalWidth();

  RefPtr<NGConstraintSpace> constraint_space =
      NGConstraintSpace::CreateFromLayoutObject(*this, override_logical_width,
                                                override_logical_height);

  RefPtr<NGLayoutResult> result = NGBlockNode(this).Layout(*constraint_space);

  // We need to update our margins as these are calculated once and stored in
  // LayoutBox::margin_box_outsets_. Typically this happens within
  // UpdateLogicalWidth and UpdateLogicalHeight.
  //
  // This primarily fixes cases where we are embedded inside another layout,
  // for example LayoutView, LayoutFlexibleBox, etc.
  UpdateMargins(*constraint_space);

  for (NGOutOfFlowPositionedDescendant descendant :
       result->OutOfFlowPositionedDescendants())
    descendant.node.UseOldOutOfFlowPositioning();

  NGPhysicalBoxFragment* fragment =
      ToNGPhysicalBoxFragment(result->PhysicalFragment().Get());

  // This object has already been positioned in legacy layout by our containing
  // block. Copy the position and place the fragment.
  const LayoutBlock* containing_block = ContainingBlock();
  NGPhysicalOffset physical_offset;
  if (containing_block) {
    NGPhysicalSize containing_block_size(containing_block->Size().Width(),
                                         containing_block->Size().Height());
    NGLogicalOffset logical_offset(LogicalLeft(), LogicalTop());
    physical_offset = logical_offset.ConvertToPhysical(
        constraint_space->WritingMode(), constraint_space->Direction(),
        containing_block_size, fragment->Size());
  }
  fragment->SetOffset(physical_offset);

  physical_root_fragment_ = fragment;
}

void LayoutNGTableCell::UpdateMargins(
    const NGConstraintSpace& constraint_space) {
  NGBoxStrut margins =
      ComputeMargins(constraint_space, StyleRef(),
                     constraint_space.WritingMode(), StyleRef().Direction());
  SetMarginBefore(margins.block_start);
  SetMarginAfter(margins.block_end);
  SetMarginStart(margins.inline_start);
  SetMarginEnd(margins.inline_end);
}

LayoutUnit LayoutNGTableCell::FirstLineBoxBaseline() const {
  // TODO(kojii): Implement. This will stop working once we stop creating line
  // boxes.
  return LayoutTableCell::FirstLineBoxBaseline();
}

LayoutUnit LayoutNGTableCell::InlineBlockBaseline(
    LineDirectionMode line_direction) const {
  // TODO(kojii): Implement. This will stop working once we stop creating line
  // boxes.
  return LayoutTableCell::InlineBlockBaseline(line_direction);
}

void LayoutNGTableCell::PaintObject(const PaintInfo& paint_info,
                                    const LayoutPoint& paint_offset) const {
  // TODO(eae): This logic should go in Paint instead and it should drive the
  // full paint logic for LayoutNGBlockFlow.
  if (RuntimeEnabledFeatures::LayoutNGPaintFragmentsEnabled())
    NGBlockFlowPainter(*this).PaintContents(paint_info, paint_offset);
  else
    LayoutTableCell::PaintObject(paint_info, paint_offset);
}

}  // namespace blink
