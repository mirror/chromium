// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/ng_physical_box_fragment.h"

#include "core/layout/LayoutBox.h"
#include "core/layout/LayoutObject.h"

namespace blink {

NGPhysicalBoxFragment::NGPhysicalBoxFragment(
    LayoutObject* layout_object,
    const ComputedStyle& style,
    NGPhysicalSize size,
    Vector<scoped_refptr<NGPhysicalFragment>>& children,
    const NGPhysicalOffsetRect& contents_visual_rect,
    Vector<NGBaseline>& baselines,
    NGBoxType box_type,
    unsigned border_edges,  // NGBorderEdges::Physical
    scoped_refptr<NGBreakToken> break_token)
    : NGPhysicalContainerFragment(layout_object,
                                  style,
                                  size,
                                  kFragmentBox,
                                  children,
                                  contents_visual_rect,
                                  std::move(break_token)),
      baselines_(std::move(baselines)) {
  DCHECK(baselines.IsEmpty());  // Ensure move semantics is used.
  box_type_ = box_type;
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

bool NGPhysicalBoxFragment::HasOverflowClip() const {
  DCHECK(GetLayoutObject());
  return GetLayoutObject()->HasOverflowClip() && BoxType() != kAnonymousBox;
}

bool NGPhysicalBoxFragment::ShouldClipOverflow() const {
  DCHECK(GetLayoutObject());
  DCHECK(GetLayoutObject()->IsBox());
  return ToLayoutBox(GetLayoutObject())->ShouldClipOverflow() &&
         BoxType() != kAnonymousBox;
}

NGPhysicalOffsetRect NGPhysicalBoxFragment::LocalVisualRect() const {
  const ComputedStyle& style = Style();
  if (!style.HasVisualOverflowingEffect())
    return {{}, Size()};

  LayoutObject* layout_object = GetLayoutObject();
  if (layout_object->IsBox()) {
    // TODO(kojii): Should move the logic to a common place.
    LayoutRect visual_rect({}, Size().ToLayoutSize());
    visual_rect.Expand(
        ToLayoutBox(layout_object)->ComputeVisualEffectOverflowOutsets());
    return NGPhysicalOffsetRect(visual_rect);
  }

  // TODO(kojii): Implement for inline boxes.
  DCHECK(layout_object->IsLayoutInline());
  return {{}, Size()};
}

NGPhysicalOffsetRect NGPhysicalBoxFragment::LocalVisualRectWithContents()
    const {
  if (HasOverflowClip() || Style().HasMask())
    return LocalVisualRect();

  NGPhysicalOffsetRect visual_rect = LocalVisualRect();
  visual_rect.Unite(ContentsVisualRect());
  return visual_rect;
}

scoped_refptr<NGPhysicalFragment> NGPhysicalBoxFragment::CloneWithoutOffset()
    const {
  Vector<scoped_refptr<NGPhysicalFragment>> children_copy(children_);
  Vector<NGBaseline> baselines_copy(baselines_);
  scoped_refptr<NGPhysicalFragment> physical_fragment =
      base::AdoptRef(new NGPhysicalBoxFragment(
          layout_object_, Style(), size_, children_copy, contents_visual_rect_,
          baselines_copy, BoxType(), border_edge_, break_token_));
  return physical_fragment;
}

}  // namespace blink
