// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/accessibility/AXPosition.h"

#include "core/dom/AXObjectCache.h"
#include "modules/accessibility/AXObject.h"
#include "modules/accessibility/AXObjectCacheImpl.h"

namespace blink {

template<typename Strategy> AXPosition::AXPosition(const PositionTemplate<Strategy>& position) : AXPosition() {
  if (position.IsNull() || position.IsOrphan())
    return;

  const PositionTemplate<Strategy>& parent_anchored_position = position.ParentAnchoredEquivalent();
  
  const PositionTemplate<Strategy> parent_anchored_position = position.
  affinity_ = selection.Affinity();
  const PositionTemplate<Strategy>& parent_anchored_position = position.ParentAnchoredEquivalent();
  Document* document = parent_anchored_position.GetDocument();
  if (!document)
    return;
  AXObjectCache* ax_object_cache = document->AxObjectCache();
  if (!ax_object_cache)
    return;

  auto* ax_object_cache_impl = static_cast<AXObjectCacheImpl*>(ax_object_cache);
  anchor_object_ = ax_object_cache_impl->GetOrCreate(parent_anchored_position.AnchorNode());
  offset_ = parent_anchored_position.OffsetInContainerNode();
}

bool AXPosition::IsValid() const {
  if (!anchor_object_ || !offset_)
    return false;
  if (anchor_object_->IsDetached())
    return false;
  if (!anchor_object_->GetNode() || !anchor_object_->GetNode()->isConnected())
    return false;
  return true;
}

PositionInFlatTreeWithAffinity AXPosition::AsPositionInFlatTreeWithAffinity() const {
  if (!IsValid())
    return {};

  PositionInFlatTree position = PositionInFlatTree::EditingPositionOf(
      anchor_object_->GetNode(), *offset_);
  return PositionInFlatTreeWithAffinity(position, affinity_);
}

}  // namespace blink
