// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/ng/ng_paint_fragment.h"

#include "core/layout/ng/layout_ng_block_flow.h"
#include "core/layout/ng/ng_physical_container_fragment.h"
#include "core/layout/ng/ng_physical_fragment.h"
#include "platform/runtime_enabled_features.h"
#include "platform/wtf/PtrUtil.h"

namespace blink {

NGPaintFragment::NGPaintFragment(RefPtr<const NGPhysicalFragment> fragment)
    : physical_fragment_(std::move(fragment)) {
  DCHECK(physical_fragment_);
}

// Populate descendant NGPaintFragment from NGPhysicalFragment tree.
void NGPaintFragment::PopulateDescendants() {
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

      if (RuntimeEnabledFeatures::SlimmingPaintV2Enabled()) {
        // For SPv2, VisaulRect() is in the space of the parent transform node.
        bool is_transform_node = false;  // TODO(kojii): how to implement?
        if (is_transform_node) {
          child->PopulateDescendants({});
        } else {
          child->PopulateDescendants(paint_offset);
          LayoutRect child_visual_rect = child->VisualRect();
          child_visual_rect.Move(child_fragment->Offset().left,
                                 child_fragment->Offset().top);
          visual_rect.Unite(child_visual_rect);
        }
      } else {
        // TODO(kojii): Add offset by
        // OffsetFromLayoutObjectWithSubpixelAccumulation()?
        child->PopulateDescendants(paint_offset);
        LayoutRect child_visual_rect = child->VisualRect();
        child_visual_rect.Move(child_fragment->Offset().left,
                               child_fragment->Offset().top);
        visual_rect.Unite(child_visual_rect);
      }
      children_.push_back(std::move(child));
    }
  }

  visual_rect.MoveBy(paint_offset);
  SetVisualRect(visual_rect);
  CopyVisualRectToLayoutObject();
}

void NGPaintFragment::CopyVisualRectToLayoutObject() {
  // TODO(kojii): Probably we need to copy only to root fragments if
  // RuntimeEnabledFeatures::LayoutNGPaintFragmentsEnabled(), or to all box
  // fragments otherwise?
  if (PhysicalFragment().IsBox()) {
    LayoutObject* layout_object = PhysicalFragment().GetLayoutObject();
    if (layout_object->IsLayoutNGBlockFlow()) {
      LayoutNGBlockFlow* block_flow = ToLayoutNGBlockFlow(layout_object);
      // ComputeOverflow() should have ran, but it didn't take visual overflow
      // of line boxes into account yet because RootInlineBox does not exist.
      // Since ComputeOverflow() reset the overflow and accumulated all what it
      // knows about, adding VisualRect() should include visual overflow from
      // inline fragments.
      block_flow->AddContentsVisualOverflow(VisualRect());
    }
  }
}

}  // namespace blink
