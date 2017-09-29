// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/ng/ng_paint_fragment.h"

#include "core/layout/ng/ng_physical_container_fragment.h"
#include "core/layout/ng/ng_physical_fragment.h"
#include "platform/wtf/PtrUtil.h"

namespace blink {

NGPaintFragment::NGPaintFragment(RefPtr<const NGPhysicalFragment> fragment)
    : physical_fragment_(std::move(fragment)) {
  DCHECK(physical_fragment_);

  // TODO(kojii): Investigate how to compute the paint origin for the root
  // fragment.
  PopulateDescendants({});
}

// Populate descendant NGPaintFragment from NGPhysicalFragment tree.
void NGPaintFragment::PopulateDescendants(LayoutPoint paint_offset) {
  LayoutRect visual_rect(PhysicalFragment().LocalVisualRectForSelf());

  if (PhysicalFragment().IsContainer()) {
    const NGPhysicalContainerFragment& fragment =
        ToNGPhysicalContainerFragment(PhysicalFragment());
    children_.ReserveCapacity(fragment.Children().size());
    for (const auto& child_fragment : fragment.Children()) {
      auto child = WTF::MakeUnique<NGPaintFragment>(child_fragment);

      LayoutRect child_visual_rect = child->VisualRect();
      child_visual_rect.Move(child_fragment->Offset().left, child_fragment->Offset().top);
      visual_rect.Unite(child_visual_rect);

      children_.push_back(std::move(child));
    }
  }

  // TODO(kojii): Do some stuff to accumulate visual rects and convert to paint
  // coordinates.
  visual_rect.MoveBy(paint_offset);
  SetVisualRect(visual_rect);
}

}  // namespace blink
