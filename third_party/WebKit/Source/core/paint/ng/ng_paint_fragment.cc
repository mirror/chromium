// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/ng/ng_paint_fragment.h"

#include "core/layout/LayoutObject.h"
#include "core/layout/ng/inline/ng_physical_text_fragment.h"
#include "core/layout/ng/ng_physical_container_fragment.h"
#include "core/layout/ng/ng_physical_fragment.h"
#include "platform/wtf/PtrUtil.h"

namespace blink {

NGPaintFragment::NGPaintFragment(RefPtr<const NGPhysicalFragment> fragment)
    : physical_fragment_(std::move(fragment)) {
  DCHECK(physical_fragment_);
}

void NGPaintFragment::UpdateVisualRect() {
  UpdateVisualRect({});
}

void NGPaintFragment::UpdateVisualRect(const UpdateContext& context) {
  const NGPhysicalFragment& fragment = PhysicalFragment();
  switch (fragment.Type()) {
    case NGPhysicalFragment::kFragmentBox: {
      LayoutObject* layout_object = fragment.GetLayoutObject();
      DCHECK(layout_object);
      SetVisualRect(layout_object->VisualRect());
      PopulateDescendants(
          {layout_object, layout_object == context.parent_box
                              ? context.offset_to_parent_box + fragment.Offset()
                              : NGPhysicalOffset()});
      return;
    }

    case NGPhysicalFragment::kFragmentText: {
      LayoutRect visual_rect = fragment.LocalVisualRect();
      // TODO(kojii): Need to transform for writing-mode.
      DCHECK(context.parent_box);
      visual_rect.MoveBy(context.parent_box->PaintOffset());
      NGPhysicalOffset offset =
          context.offset_to_parent_box + fragment.Offset();
      visual_rect.Move(offset.left, offset.top);
      SetVisualRect(visual_rect);
      return;
    }

    case NGPhysicalFragment::kFragmentLineBox: {
      NGPhysicalOffset offset =
          context.offset_to_parent_box + fragment.Offset();
      SetVisualRect({offset.left, offset.top, fragment.Size().width,
                     fragment.Size().height});
      PopulateDescendants({context.parent_box, offset});
      return;
    }
  }
  NOTREACHED();
}

// Populate descendant NGPaintFragment from NGPhysicalFragment tree.
void NGPaintFragment::PopulateDescendants(const UpdateContext& context) {
  DCHECK(children_.IsEmpty());
  DCHECK(PhysicalFragment().IsContainer());

  const NGPhysicalContainerFragment& fragment =
      ToNGPhysicalContainerFragment(PhysicalFragment());
  children_.ReserveCapacity(fragment.Children().size());
  for (const auto& child_fragment : fragment.Children()) {
    auto child = WTF::MakeUnique<NGPaintFragment>(child_fragment);
    child->UpdateVisualRect(context);
    children_.push_back(std::move(child));
  }
}

}  // namespace blink
