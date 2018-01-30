// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/ng/ng_paint_fragment.h"

#include "core/layout/LayoutInline.h"
#include "core/layout/ng/geometry/ng_physical_offset_rect.h"
#include "core/layout/ng/inline/ng_physical_text_fragment.h"
#include "core/layout/ng/ng_physical_box_fragment.h"
#include "core/layout/ng/ng_physical_container_fragment.h"
#include "core/layout/ng/ng_physical_fragment.h"
#include "core/paint/ng/ng_box_fragment_painter.h"
#include "platform/wtf/PtrUtil.h"

namespace blink {

NGPaintFragment::NGPaintFragment(
    scoped_refptr<const NGPhysicalFragment> fragment,
    NGPaintFragment* parent)
    : physical_fragment_(std::move(fragment)), parent_(parent) {
  DCHECK(physical_fragment_);
}

std::unique_ptr<NGPaintFragment> NGPaintFragment::Create(
    scoped_refptr<const NGPhysicalFragment> fragment) {
  std::unique_ptr<NGPaintFragment> paint_fragment =
      std::make_unique<NGPaintFragment>(std::move(fragment), nullptr);
  HashMap<LayoutObject*, NGPaintFragment*> layout_object_map;
  paint_fragment->PopulateDescendants(NGPhysicalOffset(), &layout_object_map);
  return paint_fragment;
}

bool NGPaintFragment::HasSelfPaintingLayer() const {
  return physical_fragment_->IsBox() &&
         ToNGPhysicalBoxFragment(*physical_fragment_).HasSelfPaintingLayer();
}

bool NGPaintFragment::HasOverflowClip() const {
  return physical_fragment_->IsBox() &&
         ToNGPhysicalBoxFragment(*physical_fragment_).HasOverflowClip();
}

bool NGPaintFragment::ShouldClipOverflow() const {
  return physical_fragment_->IsBox() &&
         ToNGPhysicalBoxFragment(*physical_fragment_).ShouldClipOverflow();
}

LayoutRect NGPaintFragment::VisualOverflowRect() const {
  return physical_fragment_->VisualRectWithContents().ToLayoutRect();
}

// Populate descendants from NGPhysicalFragment tree.
//
// |stop_at_block_layout_root| is set to |false| on the root fragment because
// most root fragments are block layout root but their children are needed.
// For children, it is set to |true| to stop at the block layout root.
void NGPaintFragment::PopulateDescendants(
    const NGPhysicalOffset inline_offset_to_container_box,
    HashMap<LayoutObject*, NGPaintFragment*>* layout_object_map) {
  DCHECK(children_.IsEmpty());
  const NGPhysicalFragment& fragment = PhysicalFragment();
  if (!fragment.IsContainer())
    return;

  // Recurse chlidren, except when this is a block layout root.
  if (fragment.IsBlockLayoutRoot() && parent_)
    return;

  const NGPhysicalContainerFragment& container =
      ToNGPhysicalContainerFragment(fragment);
  children_.ReserveCapacity(container.Children().size());
  for (const auto& child_fragment : container.Children()) {
    std::unique_ptr<NGPaintFragment> child =
        std::make_unique<NGPaintFragment>(child_fragment, this);

    if (LayoutObject* layout_object = child_fragment->GetLayoutObject()) {
      auto add_result = layout_object_map->insert(layout_object, child.get());
      // TODO(kojii): Create a list per LayoutText/LayoutInline and store to
      // them.
      // TODO: incomplete...
      if (add_result.is_new_entry) {
      } else {
        DCHECK(add_result.stored_value->value);
        add_result.stored_value->value->next_fragment_ = child.get();
        add_result.stored_value->value = child.get();
      }
    }

    child->inline_offset_to_container_box_ =
        inline_offset_to_container_box + child_fragment->Offset();
    child->PopulateDescendants(child->inline_offset_to_container_box_,
                               layout_object_map);

    children_.push_back(std::move(child));
  }
}

// TODO(kojii): This code copies VisualRects from the LayoutObject tree. We
// should implement the pre-paint tree walk on the NGPaintFragment tree, and
// save this work.
void NGPaintFragment::UpdateVisualRectFromLayoutObject() {
  DCHECK_EQ(PhysicalFragment().Type(), NGPhysicalFragment::kFragmentBox);

  UpdateVisualRectFromLayoutObject({});
}

void NGPaintFragment::UpdateVisualRectFromLayoutObject(
    const NGPhysicalOffset& parent_paint_offset) {
  // Compute VisualRect from fragment if:
  // - Text fragment, including generated content
  // - Line box fragment (does not have LayoutObject)
  // - Box fragment for inline box
  // VisualRect() of LayoutText/LayoutInline is the union of corresponding
  // fragments.
  const NGPhysicalFragment& fragment = PhysicalFragment();
  const LayoutObject* layout_object = fragment.GetLayoutObject();
  bool is_inline =
      fragment.IsText() || fragment.IsLineBox() ||
      (fragment.IsBox() && layout_object && layout_object->IsLayoutInline());
  NGPhysicalOffset paint_offset;
  if (is_inline) {
    NGPhysicalOffsetRect visual_rect = fragment.SelfVisualRect();
    paint_offset = parent_paint_offset + fragment.Offset();
    visual_rect.offset += paint_offset;
    SetVisualRect(visual_rect.ToLayoutRect());
  } else {
    // Copy the VisualRect from the corresponding LayoutObject.
    // PaintInvalidator has set the correct VisualRect to LayoutObject, computed
    // from SelfVisualRect().
    // TODO(kojii): The relationship of fragment_data and NG multicol isn't
    // clear yet. For now, this copies from the union of fragment visual rects.
    // This should be revisited if this code lives long, but the hope is for
    // PaintInvalidator to walk NG fragment tree is not that far.
    DCHECK(layout_object && layout_object->IsBox());
    DCHECK_EQ(fragment.Type(), NGPhysicalFragment::kFragmentBox);
    const DisplayItemClient* client = layout_object;
    SetVisualRect(client->VisualRect());
  }

  if (!children_.IsEmpty()) {
    // |VisualRect| of children of an inline box are relative to their inline
    // formatting context. Accumulate offset to convert to the |VisualRect|
    // space.
    if (!is_inline) {
      DCHECK(layout_object);
      paint_offset =
          NGPhysicalOffset{layout_object->FirstFragment().PaintOffset()};
    }
    for (auto& child : children_) {
      child->UpdateVisualRectFromLayoutObject(paint_offset);
    }
  }
}

void NGPaintFragment::AddSelfOutlineRect(
    Vector<LayoutRect>* outline_rects,
    const LayoutPoint& additional_offset) const {
  DCHECK(outline_rects);
  //
  LayoutRect outline_rect(additional_offset, Size().ToLayoutSize());
  // LayoutRect outline_rect = VisualRect();
  // outline_rect.MoveBy(additional_offset);
  // outline_rect.Inflate(-Style().OutlineOffset());
  // outline_rect.Inflate(-Style().OutlineWidth());

  outline_rects->push_back(outline_rect);
}

void NGPaintFragment::PaintInlineBoxForDescendants(
    const PaintInfo& paint_info,
    const LayoutPoint& paint_offset,
    const LayoutInline* layout_object,
    NGPhysicalOffset offset) const {
  for (const auto& child : Children()) {
    if (child->GetLayoutObject() == layout_object) {
      NGBoxFragmentPainter(*child).PaintInlineBox(
          paint_info, paint_offset + offset.ToLayoutPoint(), paint_offset);
      continue;
    }

    child->PaintInlineBoxForDescendants(paint_info, paint_offset, layout_object,
                                        offset + child->Offset());
  }
}

}  // namespace blink
