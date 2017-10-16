// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/ng_physical_container_fragment.h"

#include "core/layout/ng/geometry/ng_physical_offset_rect.h"
#include "core/layout/ng/inline/ng_physical_text_fragment.h"
#include "core/layout/ng/layout_ng_block_flow.h"
#include "core/layout/ng/ng_physical_box_fragment.h"
#include "core/layout/LayoutObject.h"

namespace blink {

namespace {

#if DCHECK_IS_ON()
void CheckVisualRectIsEmpty(const NGPhysicalContainerFragment& fragment) {
  for (const auto& child : fragment.Children()) {
    LayoutObject* layout_object = child->GetLayoutObject();
    DCHECK(layout_object);
    DCHECK(layout_object->VisualRect().IsEmpty());
    if (child->IsContainer())
      CheckVisualRectIsEmpty(ToNGPhysicalContainerFragment(*child));
  }
}
#endif

void UniteToVisualRect(const NGPhysicalOffsetRect& rect, LayoutObject* layout_object) {
  LayoutRect layout_rect = rect.ToLayoutRect();
  layout_rect.Unite(layout_object->VisualRect());
  layout_object->GetMutableForPainting().SetVisualRect(layout_rect);
}

}  // namespace

NGPhysicalContainerFragment::NGPhysicalContainerFragment(
    LayoutObject* layout_object,
    const ComputedStyle& style,
    NGPhysicalSize size,
    NGFragmentType type,
    Vector<RefPtr<NGPhysicalFragment>>& children,
    RefPtr<NGBreakToken> break_token)
    : NGPhysicalFragment(layout_object,
                         style,
                         size,
                         type,
                         std::move(break_token)),
      children_(std::move(children)) {
  DCHECK(children.IsEmpty());  // Ensure move semantics is used.
}

void NGPhysicalContainerFragment::ComputeLocalVisualRectForInlineChildren() const {
#if 0
#if DCHECK_IS_ON()
  CheckVisualRectIsEmpty(*this);
#endif
  ComputeLocalVisualRectForInlineChildren({});
#endif
}

void NGPhysicalContainerFragment::ComputeLocalVisualRectForInlineChildren(const NGPhysicalOffset offset_to_containing_box) const {
  for (const auto& child : Children()) {
    LayoutObject* layout_object = child->GetLayoutObject();
    DCHECK(layout_object);
    NGPhysicalOffset child_offset_to_containing_box =
        offset_to_containing_box + child->Offset();

    if (child->IsText()) {
      DCHECK(layout_object->IsText());
      const NGPhysicalTextFragment& text_child = ToNGPhysicalTextFragment(*child);
      UniteToVisualRect(
          text_child.LocalVisualRect() + child_offset_to_containing_box,
          layout_object);
      continue;
    }

    if (layout_object->IsAtomicInlineLevel() ||
        layout_object->IsFloatingOrOutOfFlowPositioned())
      continue;
    DCHECK(!layout_object->IsLayoutNGBlockFlow() ||
           !ToLayoutNGBlockFlow(layout_object)->HasNGInlineNodeData());

    if (layout_object->IsLayoutInline()) {
      DCHECK(child->IsBox());
      const NGPhysicalBoxFragment& box_child = ToNGPhysicalBoxFragment(*child);
      UniteToVisualRect(
          box_child.LocalVisualRect() + child_offset_to_containing_box,
          layout_object);
    }

    DCHECK(child->IsContainer());
    ToNGPhysicalContainerFragment(*child)
        .ComputeLocalVisualRectForInlineChildren(
            child_offset_to_containing_box);
  }
}

}  // namespace blink
