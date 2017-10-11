// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/ng/ng_paint_fragment.h"

#include "core/layout/LayoutObject.h"
#include "core/layout/ng/geometry/ng_physical_rect.h"
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
  if (!children_.IsEmpty())
    return;

  UpdateVisualRect({nullptr});
}

void NGPaintFragment::UpdateVisualRect(const UpdateContext& context) {
  const NGPhysicalFragment& fragment = PhysicalFragment();
  switch (fragment.Type()) {
    case NGPhysicalFragment::kFragmentBox: {
      // For box fragments, LayoutBox has the correct LocalVisualRect() and
      // that VisualRect() is set correctly by PaintInvalidator.
      // Copy the VisualRect from the LayoutObject.
      LayoutObject* layout_object = fragment.GetLayoutObject();
      DCHECK(layout_object);
      if (layout_object == context.parent_box) {
        // If this is an anonymous fragment; i.e., does not exist in
        // LayoutObject tree, it shouldn't paint anything.
        // Use the same VisualRect() as its parent box.
        SetVisualRect(layout_object->VisualRect());
        PopulateDescendants(
            {layout_object, context.offset_to_parent_box + fragment.Offset()});
      } else {
        // TODO(kojii): For LayoutInline, LocalVisualRect() walks InlineBox
        // tree, and thus incorrect at this point.
        SetVisualRect(layout_object->VisualRect());
        PopulateDescendants({layout_object});
      }
      return;
    }

    case NGPhysicalFragment::kFragmentText: {
      // TODO(kojii): This code computes VisualRect for this text fragment, but
      // 1) does not update LayoutText::VisualRect(), and 2) is too late because
      // paint invalidation is done.
      NGPhysicalRect visual_rect =
          ToNGPhysicalTextFragment(fragment).LocalVisualRect();
      DCHECK(context.parent_box);
      visual_rect.location +=
          fragment.Offset() + context.offset_to_parent_box +
          NGPhysicalOffset(context.parent_box->PaintOffset());
      SetVisualRect(visual_rect.ToLayoutRect());
      return;
    }

    case NGPhysicalFragment::kFragmentLineBox: {
      // Line box fragments do not paint anything.
      NGPhysicalOffset offset =
          context.offset_to_parent_box + fragment.Offset();
      SetVisualRect({offset.left, offset.top, fragment.Size().width,
                     fragment.Size().height});
      PopulateDescendants({context.parent_box,
                           context.offset_to_parent_box + fragment.Offset()});
      return;
    }
  }
  NOTREACHED();
}

// Populate descendants from NGPhysicalFragment tree.
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
