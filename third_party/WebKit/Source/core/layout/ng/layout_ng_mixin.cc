// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/layout_ng_mixin.h"

#include "core/layout/LayoutBlockFlow.h"
#include "core/layout/LayoutTableCell.h"
#include "core/layout/ng/inline/ng_inline_node_data.h"
#include "core/layout/ng/ng_constraint_space.h"
#include "core/layout/ng/ng_layout_result.h"
#include "core/layout/ng/ng_length_utils.h"
#include "core/paint/ng/ng_block_flow_painter.h"

namespace blink {

template <typename BASE>
LayoutNGMixin<BASE>::~LayoutNGMixin() {}

template <typename BASE>
bool LayoutNGMixin<BASE>::IsOfType(LayoutObject::LayoutObjectType type) const {
  return type == LayoutObject::kLayoutObjectNGMixin || BASE::IsOfType(type);
}

template <typename BASE>
NGInlineNodeData* LayoutNGMixin<BASE>::GetNGInlineNodeData() const {
  DCHECK(ng_inline_node_data_);
  return ng_inline_node_data_.get();
}

template <typename BASE>
void LayoutNGMixin<BASE>::ResetNGInlineNodeData() {
  ng_inline_node_data_ = WTF::MakeUnique<NGInlineNodeData>();
}

// The current fragment from the last layout cycle for this box.
// When pre-NG layout calls functions of this block flow, fragment and/or
// LayoutResult are required to compute the result.
// TODO(kojii): Use the cached result for now, we may need to reconsider as the
// cache evolves.
template <typename BASE>
const NGPhysicalFragment* LayoutNGMixin<BASE>::CurrentFragment() const {
  if (cached_result_)
    return cached_result_->PhysicalFragment().get();
  return nullptr;
}

template <typename BASE>
void LayoutNGMixin<BASE>::AddOverflowFromChildren() {
  // |ComputeOverflow()| calls this, which is called from
  // |CopyFragmentDataToLayoutBox()| and |RecalcOverflowAfterStyleChange()|.
  // Add overflow from the last layout cycle.
  if (BASE::ChildrenInline()) {
    if (const NGPhysicalFragment* physical_fragment = CurrentFragment()) {
      // TODO(kojii): If |RecalcOverflowAfterStyleChange()|, we need to
      // re-compute glyph bounding box. How to detect it and how to re-compute
      // is TBD.
      LayoutRect visual_rect = physical_fragment->LocalVisualRect();
      BASE::AddContentsVisualOverflow(visual_rect);
      // TODO(kojii): The above code computes visual overflow only, we fallback
      // to LayoutBlock for AddLayoutOverflow() for now. It doesn't compute
      // correctly without RootInlineBox though.
    }
  }
  BASE::AddOverflowFromChildren();
}

// Retrieve NGBaseline from the current fragment.
template <typename BASE>
const NGBaseline* LayoutNGMixin<BASE>::FragmentBaseline(
    NGBaselineAlgorithmType type) const {
  if (const NGPhysicalFragment* physical_fragment = CurrentFragment()) {
    FontBaseline baseline_type = BASE::IsHorizontalWritingMode()
                                     ? kAlphabeticBaseline
                                     : kIdeographicBaseline;
    return ToNGPhysicalBoxFragment(physical_fragment)
        ->Baseline({type, baseline_type});
  }
  return nullptr;
}

template <typename BASE>
LayoutUnit LayoutNGMixin<BASE>::FirstLineBoxBaseline() const {
  if (BASE::ChildrenInline()) {
    if (const NGBaseline* baseline =
            FragmentBaseline(NGBaselineAlgorithmType::kFirstLine)) {
      return baseline->offset;
    }
  }
  return BASE::FirstLineBoxBaseline();
}

template <typename BASE>
LayoutUnit LayoutNGMixin<BASE>::InlineBlockBaseline(
    LineDirectionMode line_direction) const {
  if (BASE::ChildrenInline()) {
    if (const NGBaseline* baseline =
            FragmentBaseline(NGBaselineAlgorithmType::kAtomicInline)) {
      return baseline->offset;
    }
  }
  return BASE::InlineBlockBaseline(line_direction);
}

template <typename BASE>
RefPtr<NGLayoutResult> LayoutNGMixin<BASE>::CachedLayoutResult(
    const NGConstraintSpace& constraint_space,
    NGBreakToken* break_token) const {
  if (!cached_result_ || break_token || BASE::NeedsLayout())
    return nullptr;
  if (constraint_space != *cached_constraint_space_)
    return nullptr;
  return cached_result_->CloneWithoutOffset();
}

template <typename BASE>
void LayoutNGMixin<BASE>::SetCachedLayoutResult(
    const NGConstraintSpace& constraint_space,
    NGBreakToken* break_token,
    RefPtr<NGLayoutResult> layout_result) {
  if (!RuntimeEnabledFeatures::LayoutNGFragmentCachingEnabled())
    return;
  if (break_token || constraint_space.UnpositionedFloats().size() ||
      layout_result->UnpositionedFloats().size() ||
      layout_result->Status() != NGLayoutResult::kSuccess) {
    // We can't cache these yet
    return;
  }

  cached_constraint_space_ = &constraint_space;
  cached_result_ = layout_result;
}

template <typename BASE>
void LayoutNGMixin<BASE>::PaintObject(const PaintInfo& paint_info,
                                      const LayoutPoint& paint_offset) const {
  // TODO(eae): This logic should go in Paint instead and it should drive the
  // full paint logic for LayoutNGBlockFlow.
  if (RuntimeEnabledFeatures::LayoutNGPaintFragmentsEnabled())
    NGBlockFlowPainter(*this).PaintContents(paint_info, paint_offset);
  else
    BASE::PaintObject(paint_info, paint_offset);
}

template <typename BASE>
void LayoutNGMixin<BASE>::UpdateMargins(
    const NGConstraintSpace& constraint_space) {
  BASE::SetMargin(ComputePhysicalMargins(constraint_space, BASE::StyleRef()));
}

template class LayoutNGMixin<LayoutTableCell>;
template class LayoutNGMixin<LayoutBlockFlow>;

}  // namespace blink
