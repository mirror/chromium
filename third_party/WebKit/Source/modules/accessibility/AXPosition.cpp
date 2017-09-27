// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/accessibility/AXPosition.h"

#include "core/dom/AXObjectCache.h"
#include "core/dom/Document.h"
#include "core/dom/Node.h"
#include "core/editing/EphemeralRange.h"
#include "core/editing/iterators/CharacterIterator.h"
#include "core/editing/iterators/TextIterator.h"
#include "modules/accessibility/AXObject.h"
#include "modules/accessibility/AXObjectCacheImpl.h"

namespace blink {

AXPosition::AXPosition()
    : anchor_object_(nullptr), offset_(), affinity_(TextAffinity::kDownstream) {
#if DCHECK_IS_ON()
  dom_tree_version_ = 0;
#endif
}

AXPosition::AXPosition(AXObject* anchor_object,
                       int offset,
                       TextAffinity affinity)
    : AXPosition(),
      anchor_object_(anchor_object),
      offset_(offset),
      affinity_(affinity) {
  if (anchor_object_ && anchor_object_->GetDocument()) {
    const Document* document = anchor_object_->GetDocument();
    DCHECK(!document->NeedsLayoutTreeUpdate());
#if DCHECK_IS_ON()
    dom_tree_version_ = document->DomTreeVersion();
#endif
  }
}

AXPosition::AXPosition(const Position& position) : AXPosition() {
  if (position.IsNull() || position.IsOrphan())
    return;

  const Position& parent_anchored_position = position.ToOffsetInAnchor();
  const Document* document = parent_anchored_position.GetDocument();
  // Non orphan positions always have a document.
  DCHECK(document);
  DCHECK(!document->NeedsLayoutTreeUpdate());
#if DCHECK_IS_ON()
  dom_tree_version_ = document->DomTreeVersion();
#endif

  AXObjectCache* ax_object_cache = document->AxObjectCache();
  if (!ax_object_cache)
    return;

  auto* ax_object_cache_impl = static_cast<AXObjectCacheImpl*>(ax_object_cache);
  Const Node* anchor_node = parent_anchored_position.AnchorNode();
  DCHECK(anchor_node);
  anchor_object_ = ax_object_cache_impl->GetOrCreate(anchor_node);
  if (anchor_node->IsTextNode()) {
    // Convert from a DOM offset that may have uncompressed white space to a
    // character offset.
    const auto first_position = Position::FirstPositionInNode(anchor_node);
    const EphemeralRange range(first_position, parent_anchored_position);
    offset_ = TextIterator::RangeLength(range);
  } else {
    // TODO(nektar): Convert from DOM child index to AX child index.
    offset_ = parent_anchored_position.OffsetInContainerNode();
  }
}

bool AXPosition::IsValid() const {
  if (!anchor_object_ || !offset_)
    return false;
  if (anchor_object_->IsDetached())
    return false;
  if (!anchor_object_->GetNode() || !anchor_object_->GetNode()->isConnected())
    return false;
  DCHECK_EQ(anchor_object_->GetNode()->GetDocument().DomTreeVersion(),
            dom_tree_version_);
  return true;
}

bool AXPosition::IsTextPosition() const {
  return IsValid() && anchor_object_->GetNode()->IsTextNode();
}

PositionWithAffinity AXPosition::AsPositionWithAffinity() const {
  if (!IsValid())
    return {};

  if (!IsTextPosition()) {
    // TODO(nektar): Convert from AX child index to DOM child index.
    const auto position = Position(anchor_object_->GetNode(), *offset_);
    return PositionWithAffinity(position, affinity_);
  }

  const auto first_position =
      Position::FirstPositionInNode(anchor_object_->GetNode());
  const auto last_position =
      Position::LastPositionInNode(anchor_object_->GetNode());
  const CharacterIterator character_iterator(first_position, last_position);
  const EphemeralRange range =
      character_iterator.CalculateCharacterSubrange(0, offset);
  return PositionWithAffinity(range.EndPosition(), affinity_);
}

}  // namespace blink
