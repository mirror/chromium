// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/ng/ng_paint_fragment.h"

#include "core/layout/LayoutObject.h"
#include "core/layout/ng/geometry/ng_physical_offset_rect.h"
#include "core/layout/ng/inline/ng_physical_text_fragment.h"
#include "core/layout/ng/ng_physical_container_fragment.h"
#include "core/layout/ng/ng_physical_fragment.h"
#include "platform/wtf/PtrUtil.h"

namespace blink {

NGPaintFragment::NGPaintFragment(
    scoped_refptr<const NGPhysicalFragment> fragment)
    : physical_fragment_(std::move(fragment)) {
  DCHECK(physical_fragment_);
}

void NGPaintFragment::UpdateVisualRectFromLayoutObject() {
  DCHECK_EQ(PhysicalFragment().Type(), NGPhysicalFragment::kFragmentBox);

  // TODO(kojii): In 2nd or further calls, should we update VisualRect without
  // re-creating children?
  if (!children_.IsEmpty())
    return;

  UpdateVisualRectFromLayoutObject({nullptr});
}

void NGPaintFragment::UpdateVisualRectFromLayoutObject(
    const UpdateContext& context) {
  // Compute VisualRect from fragment if:
  // - Text fragment, including generated content
  // - Line box fragment (does not have LayoutObject)
  // - Box fragment for inline box
  // VisualRect() of LayoutText/LayoutInline is the union of corresponding
  // fragments.
  const NGPhysicalFragment& fragment = PhysicalFragment();
  const LayoutObject* layout_object = fragment.GetLayoutObject();
  if (fragment.IsText() || !layout_object || layout_object->IsLayoutInline()) {
    DCHECK(fragment.IsText() || fragment.IsLineBox() ||
           (fragment.IsBox() && !fragment.IsAnonymousBox()));
    NGPhysicalOffsetRect visual_rect = fragment.LocalVisualRect();
    DCHECK(context.parent_box);
    visual_rect.offset += fragment.Offset() + context.offset_to_parent_box +
                          NGPhysicalOffset(context.parent_box->PaintOffset());
    SetVisualRect(visual_rect.ToLayoutRect());
  } else {
    // Copy the VisualRect from the corresponding LayoutObject.
    // PaintInvalidator has set the correct VisualRect to LayoutObject, computed
    // from LocalVisualRect().
    DCHECK(layout_object->IsBox());
    DCHECK_EQ(fragment.Type(), NGPhysicalFragment::kFragmentBox);
    SetVisualRect(layout_object->VisualRect());
  }

  // Recurse chlidren, except when this is a block layout root.
  if (fragment.IsContainer() &&
      !(fragment.IsBlockLayoutRoot() && context.parent_box)) {
    if (!layout_object || fragment.IsAnonymousBox()) {
      // If this fragment isn't from a LayoutObject; i.e., a line box or an
      // anonymous, keep the offset to the parent box.
      PopulateDescendants({context.parent_box,
                           context.offset_to_parent_box + fragment.Offset()});
    } else {
      PopulateDescendants({layout_object});
    }
  }
}

// Populate descendants from NGPhysicalFragment tree.
void NGPaintFragment::PopulateDescendants(const UpdateContext& context) {
  DCHECK(children_.IsEmpty());
  DCHECK(PhysicalFragment().IsContainer());
  const NGPhysicalContainerFragment& container =
      ToNGPhysicalContainerFragment(PhysicalFragment());
  children_.ReserveCapacity(container.Children().size());
  for (const auto& child_fragment : container.Children()) {
    auto child = WTF::MakeUnique<NGPaintFragment>(child_fragment);
    child->UpdateVisualRectFromLayoutObject(context);
    children_.push_back(std::move(child));
  }
}

}  // namespace blink
