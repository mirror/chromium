// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/ng_physical_box_fragment.h"

namespace blink {

NGPhysicalBoxFragment::NGPhysicalBoxFragment(
    LayoutObject* layout_object,
    const ComputedStyle& style,
    NGPhysicalSize size,
    const NGPhysicalOffsetRect& contents_visual_rect,
    Vector<RefPtr<NGPhysicalFragment>>& children,
    Vector<NGBaseline>& baselines,
    unsigned border_edges,  // NGBorderEdges::Physical
    RefPtr<NGBreakToken> break_token)
    : NGPhysicalContainerFragment(layout_object,
                                  style,
                                  size,
                                  kFragmentBox,
                                  children,
                                  std::move(break_token)),
      contents_visual_rect_(contents_visual_rect),
      baselines_(std::move(baselines)) {
  DCHECK(baselines.IsEmpty());  // Ensure move semantics is used.
  border_edge_ = border_edges;
}

const NGBaseline* NGPhysicalBoxFragment::Baseline(
    const NGBaselineRequest& request) const {
  for (const auto& baseline : baselines_) {
    if (baseline.request == request)
      return &baseline;
  }
  return nullptr;
}

bool NGPhysicalBoxFragment::IsBlockLayoutBoundary() const {
  // An anonymous fragment is not a boundary.
  if (!layout_object_ || layout_object_->Style() != style_.get())
    return false;
  if (layout_object_->IsAtomicInlineLevel() ||
      layout_object_->IsFloatingOrOutOfFlowPositioned())
    return true;
  return false;
}

const NGPhysicalOffsetRect NGPhysicalBoxFragment::LocalVisualRect() const {
  // TODO(kojii): Add its own visual overflow (e.g., box-shadow)
  return {{}, Size()};
}

RefPtr<NGPhysicalFragment> NGPhysicalBoxFragment::CloneWithoutOffset() const {
  Vector<RefPtr<NGPhysicalFragment>> children_copy(children_);
  Vector<NGBaseline> baselines_copy(baselines_);
  RefPtr<NGPhysicalFragment> physical_fragment =
      WTF::AdoptRef(new NGPhysicalBoxFragment(
          layout_object_, Style(), size_, contents_visual_rect_, children_copy,
          baselines_copy, border_edge_, break_token_));
  return physical_fragment;
}

}  // namespace blink
