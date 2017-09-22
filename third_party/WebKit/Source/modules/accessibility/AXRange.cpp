// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/accessibility/AXRange.h"

#include "core/dom/AXObjectCache.h"
#include "core/editing/FrameSelection.h"
#include "core/frame/LocalFrame.h"
#include "core/html/TextControlElement.h"
#include "modules/accessibility/AXObject.h"
#include "modules/accessibility/AXObjectCacheImpl.h"

namespace blink {

AXRange::AXRange(const SelectionInDOMTree& selection) : AXRange() {
  if (selection.IsNone())
    return;
  DCHECK(selection.AssertValid());
  affinity_ = selection.Affinity();
  const PositionInFlatTree& base = ToPositionInFlatTree(selection.Base());
  const PositionInFlatTree& extent = ToPositionInFlatTree(selection.Extent());
  Init(base, extent);
}

AXRange::AXRange(const SelectionInFlatTree& selection) : AXRange() {
  if (selection.IsNone())
    return;
  DCHECK(selection.AssertValid());
  affinity_ = selection.Affinity();
  Init(selection.Base(), selection.Extent());
}

bool AXRange::IsValid() const {
  if (!anchor_object_ || !focus_object_ || anchor_offset_ < 0 ||
      focus_offset_ < 0) {
    return false;
  }
  if (anchor_object_->IsDetached() || focus_object_->IsDetached())
    return false;
  if (!anchor_object_->GetNode() || !focus_object_->GetNode() ||
      !anchor_object_->GetNode()->isConnected() ||
      !focus_object_->GetNode()->isConnected()) {
    return false;
  }
  // We don't support ranges that span across documents.
  if (anchor_object_->GetDocument() != focus_object_->GetDocument())
    return false;
  return true;
}

SelectionInDOMTree AXRange::AsSelectionInDOMTree() const {
  if (!IsValid())
    return {};
  Position base =
      Position::EditingPositionOf(anchor_object_->GetNode(), anchor_offset_);
  Position extent =
      Position::EditingPositionOf(focus_object_->GetNode(), focus_offset_);
  SelectionInDOMTree::Builder selection_builder;
  selection_builder.SetBaseAndExtent(base, extent);
  selection_builder.SetAffinity(affinity_);
  return selection_builder.Build();
}

void AXRange::Select() {
  if (!IsValid())
    return;
  if (IsSimple() && IsTextControlElement(focus_object_->GetNode())) {
    TextControlElement* text_control =
        ToTextControlElement(focus_object_->GetNode());
    if (anchor_offset_ <= focus_offset_) {
      text_control->SetSelectionRange(anchor_offset_, focus_offset_,
                                      kSelectionHasForwardDirection);
    } else {
      text_control->SetSelectionRange(anchor_offset_, focus_offset_,
                                      kSelectionHasBackwardDirection);
    }
    return;
  }

  const SelectionInDOMTree selection = AsSelectionInDOMTree();
  DCHECK(selection.AssertValid());
  Document* document = selection.Base().GetDocument();
  if (!document)
    return;
  LocalFrame* frame = document->GetFrame();
  if (!frame)
    return;
  FrameSelection& frame_selection = frame->Selection();
  frame_selection.SetSelection(selection);
}

void AXRange::Init(const PositionInFlatTree& base,
                   const PositionInFlatTree& extent) {
  Document* document = base.GetDocument();
  if (!document || document != extent.GetDocument())
    return;
  AXObjectCache* ax_object_cache = document->AxObjectCache();
  if (!ax_object_cache)
    return;

  AXObjectCacheImpl* ax_object_cache_impl =
      static_cast<AXObjectCacheImpl*>(ax_object_cache);
  anchor_object_ = ax_object_cache_impl->GetOrCreate(base.AnchorNode());
  anchor_offset_ = base.OffsetInContainerNode();
  focus_object_ = ax_object_cache_impl->GetOrCreate(extent.AnchorNode());
  focus_offset_ = extent.OffsetInContainerNode();
}

}  // namespace blink
