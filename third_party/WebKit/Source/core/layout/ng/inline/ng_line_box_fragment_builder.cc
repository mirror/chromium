// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_line_box_fragment_builder.h"

#include "core/layout/ng/geometry/ng_logical_size.h"
#include "core/layout/ng/inline/ng_inline_break_token.h"
#include "core/layout/ng/inline/ng_inline_node.h"
#include "core/layout/ng/inline/ng_physical_line_box_fragment.h"
#include "core/layout/ng/ng_layout_result.h"

namespace blink {

NGLineBoxFragmentBuilder::NGLineBoxFragmentBuilder(
    NGInlineNode node,
    RefPtr<const ComputedStyle> style,
    NGWritingMode writing_mode)
    : NGContainerFragmentBuilder(style, writing_mode, TextDirection::kLtr),
      node_(node) {}

NGLogicalSize NGLineBoxFragmentBuilder::Size() const {
  return {inline_size_, metrics_.LineHeight()};
}

NGLineBoxFragmentBuilder& NGLineBoxFragmentBuilder::AddChild(
    RefPtr<NGLayoutResult> layout_result,
    const NGLogicalOffset& child_offset) {
  children_.push_back(Child{std::move(layout_result), nullptr, child_offset});
  return *this;
}

NGLineBoxFragmentBuilder& NGLineBoxFragmentBuilder::AddChild(
    RefPtr<NGPhysicalFragment> fragment,
    const NGLogicalOffset& child_offset) {
  children_.push_back(Child{nullptr, std::move(fragment), child_offset});
  return *this;
}

NGLineBoxFragmentBuilder& NGLineBoxFragmentBuilder::AddChild(
    std::nullptr_t,
    const NGLogicalOffset& child_offset) {
  children_.push_back(Child{nullptr, nullptr, child_offset});
  return *this;
}

const NGPhysicalFragment* NGLineBoxFragmentBuilder::Child::PhysicalFragment() const {
  return layout_result ? layout_result->PhysicalFragment().get()
                       : fragment.get();
}

const NGPhysicalFragment* NGLineBoxFragmentBuilder::ChildFragment(
    unsigned index) const {
  return children_[index].PhysicalFragment();
}

void NGLineBoxFragmentBuilder::SetChildInlineOffset(unsigned index, LayoutUnit inline_offset) {
  children_[index].offset.inline_offset = inline_offset;
}

void NGLineBoxFragmentBuilder::MoveChildrenInBlockDirection(LayoutUnit delta) {
  for (auto& child : children_)
    child.offset.block_offset += delta;
}

void NGLineBoxFragmentBuilder::MoveChildrenInBlockDirection(LayoutUnit delta,
                                                            unsigned start,
                                                            unsigned end) {
  for (unsigned index = start; index < end; index++)
    children_[index].offset.block_offset += delta;
}

void NGLineBoxFragmentBuilder::SetMetrics(const NGLineHeightMetrics& metrics) {
  metrics_ = metrics;
}

void NGLineBoxFragmentBuilder::SetBreakToken(
    RefPtr<NGInlineBreakToken> break_token) {
  break_token_ = std::move(break_token);
}

RefPtr<NGPhysicalLineBoxFragment>
NGLineBoxFragmentBuilder::ToLineBoxFragment() {
  DCHECK_EQ(0u, NGContainerFragmentBuilder::offsets_.size());
  DCHECK_EQ(0u, NGContainerFragmentBuilder::children_.size());

  NGWritingMode writing_mode(
      FromPlatformWritingMode(node_.Style().GetWritingMode()));
  NGPhysicalSize physical_size = Size().ConvertToPhysical(writing_mode);

  Vector<RefPtr<NGPhysicalFragment>> children;
  children.ReserveInitialCapacity(children_.size());
  for (auto& child : children_) {
    RefPtr<NGPhysicalFragment> fragment;
    if (child.layout_result) {
      fragment = std::move(child.layout_result->MutablePhysicalFragment());
    } else {
      DCHECK(child.fragment);
      fragment = std::move(child.fragment);
    }
    fragment->SetOffset(child.offset.ConvertToPhysical(
        writing_mode, Direction(), physical_size, fragment->Size()));
    children.push_back(std::move(fragment));
  }

  RefPtr<NGPhysicalLineBoxFragment> fragment =
      WTF::AdoptRef(new NGPhysicalLineBoxFragment(
          Style(), physical_size, children, metrics_,
          break_token_ ? std::move(break_token_)
                       : NGInlineBreakToken::Create(node_)));
  return fragment;
}

}  // namespace blink
