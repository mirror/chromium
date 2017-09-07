// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/ng_base_fragment_builder.h"

#include "core/layout/ng/ng_exclusion_space.h"

namespace blink {

NGBaseFragmentBuilder::NGBaseFragmentBuilder(RefPtr<const ComputedStyle> style,
                                             NGWritingMode writing_mode,
                                             TextDirection direction)
    : style_(std::move(style)),
      writing_mode_(writing_mode),
      direction_(direction) {
  DCHECK(style_);
}

NGBaseFragmentBuilder::NGBaseFragmentBuilder(NGWritingMode writing_mode,
                                             TextDirection direction)
    : writing_mode_(writing_mode), direction_(direction) {}

NGBaseFragmentBuilder::~NGBaseFragmentBuilder() {}

NGBaseFragmentBuilder& NGBaseFragmentBuilder::SetInlineSize(
    LayoutUnit size) {
  inline_size_ = size;
  return *this;
}

NGBaseFragmentBuilder& NGBaseFragmentBuilder::SetBfcOffset(const NGBfcOffset& offset) {
  bfc_offset_ = offset;
  return *this;
}

NGBaseFragmentBuilder& NGBaseFragmentBuilder::SetStyle(
    RefPtr<const ComputedStyle> style) {
  DCHECK(style);
  style_ = std::move(style);
  return *this;
}

NGBaseFragmentBuilder& NGBaseFragmentBuilder::AddOutOfFlowDescendant(
    NGOutOfFlowPositionedDescendant descendant) {
  oof_positioned_descendants_.push_back(descendant);
  return *this;
}

NGBaseFragmentBuilder& NGBaseFragmentBuilder::SetExclusionSpace(
    std::unique_ptr<const NGExclusionSpace> exclusion_space) {
  exclusion_space_ = std::move(exclusion_space);
  return *this;
}

NGBaseFragmentBuilder& NGBaseFragmentBuilder::AddChild(
    RefPtr<NGLayoutResult> child,
    const NGLogicalOffset& child_offset) {
  // Collect child's out of flow descendants.
  for (const NGOutOfFlowPositionedDescendant& descendant :
       child->OutOfFlowPositionedDescendants()) {
    oof_positioned_candidates_.push_back(
        NGOutOfFlowPositionedCandidate{descendant, child_offset});
  }

  return AddChild(child->PhysicalFragment(), child_offset);
}

NGBaseFragmentBuilder& NGBaseFragmentBuilder::AddChild(
    RefPtr<NGPhysicalFragment> child,
    const NGLogicalOffset& child_offset) {
  children_.push_back(std::move(child));
  offsets_.push_back(child_offset);

  return *this;
}

RefPtr<NGLayoutResult> NGBaseFragmentBuilder::Abort(
    NGLayoutResult::NGLayoutResultStatus status) {
  return AdoptRef(new NGLayoutResult(
      nullptr, Vector<NGOutOfFlowPositionedDescendant>(), unpositioned_floats_,
      nullptr, bfc_offset_, end_margin_strut_, status));
}

}  // namespace blink
