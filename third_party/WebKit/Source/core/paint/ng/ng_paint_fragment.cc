// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/ng/ng_paint_fragment.h"

#include "core/layout/ng/inline/ng_physical_text_fragment.h"
#include "core/layout/ng/ng_physical_container_fragment.h"
#include "core/layout/ng/ng_physical_fragment.h"
#include "platform/wtf/PtrUtil.h"

namespace blink {

NGPaintFragment::NGPaintFragment(RefPtr<const NGPhysicalFragment> fragment)
    : physical_fragment_(std::move(fragment)) {
  DCHECK(physical_fragment_);
}

// Populate descendant NGPaintFragment from NGPhysicalFragment tree.
void NGPaintFragment::PopulateDescendants() {
  DCHECK(children_.IsEmpty());

  if (PhysicalFragment().IsContainer()) {
    const NGPhysicalContainerFragment& fragment =
        ToNGPhysicalContainerFragment(PhysicalFragment());
    children_.ReserveCapacity(fragment.Children().size());
    for (const auto& child_fragment : fragment.Children()) {
      auto child = WTF::MakeUnique<NGPaintFragment>(child_fragment);
      child->PopulateDescendants();
      children_.push_back(std::move(child));
    }
  }

  UpdateVisualRect();
}

void NGPaintFragment::UpdateVisualRect() {
  const NGPhysicalFragment& fragment = PhysicalFragment();
  switch (fragment.Type()) {
    case NGPhysicalFragment::kFragmentBox:
      LayoutObject* layout_object = fragment.GetLayoutObject();
      DCHECK(layout_object);
      SetVisualRect(layout_object->VisualRect());
      return;

    case NGPhysicalFragment::kFragmentText: {
      LayoutObject* layout_object = fragment.GetLayoutObject();
      DCHECK(layout_object);
      LayoutRect visual_rect = fragment.LocalVisualRect();
      visual_rect.MoveBy(layout_object->PaintOffset());
      SetVisualRect(visual_rect);
      return;
    }

    case NGPhysicalFragment::kFragmentLineBox: {
      LayoutRect visual_rect;
      for (const auto& child : children_) {
        visual_rect.Unite(child->VisualRect());
      }
      SetVisualRect(visual_rect);
      return;
    }
  }
  NOTREACHED();
}

}  // namespace blink
