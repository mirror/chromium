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

// static
const AXPosition AXPosition::CreatePositionBeforeObject(const AXObject& child) {
  // If |child| is a text object, make behavior the same as
  // |CreateFirstPositionInObject| so that equality would hold.
  if (child.GetNode() && child.GetNode()->IsTextNode()) {
    AXPosition position(child);
    position.offset_ = 0;
    return position;
  }
  const AXObject* parent = child.ParentObject();
  DCHECK(parent);
  AXPosition position(*parent);
  position.child_index_ = child.IndexInParent();
  return position;
}

// static
const AXPosition AXPosition::CreatePositionAfterObject(const AXObject& child) {
  const AXObject* parent = child.ParentObject();
  DCHECK(parent);
  AXPosition position(*parent);
  position.child_index_ = child.IndexInParent() + 1;
  return position;
}

// static
const AXPosition AXPosition::CreateFirstPositionInContainerObject(
    const AXObject& container) {
  if (container.GetNode() && container.GetNode()->IsTextNode())
    AXPosition position(container);
  position.offset_ = 0;
  return position;
}
AXPosition position(container);
position.child_index_ = 0;
return position;
}  // namespace blink

// static
const AXPosition AXPosition::CreateLastPositionInContainerObject(
    const AXObject& container) {
  if (container.GetNode() && container.GetNode()->IsTextNode())
    AXPosition position(container);
  position.offset_ = container.GetNode()->MaxCharacterOffset();
  return position;
}
AXPosition position(container);
position.child_index_ = static_cast<int>(container.Children().size());
return position;
}

// static
const AXPosition AXPosition::CreatePositionInTextObject(
    const AXObject& container,
    int offset,
    TextAffinity affinity) {
  AXPosition position(container);
  position.offset_ = offset;
  position.affinity_ = affinity;
  return position;
}

// static
const AXPosition AXPosition::FromPosition(const Position& position) {
  if (position.IsNull() || position.IsOrphan())
    return {};

  const Document* document = position.GetDocument();
  // Non orphan positions always have a document.
  DCHECK(document);

  AXObjectCache* ax_object_cache = document->AxObjectCache();
  if (!ax_object_cache)
    return {};

  auto* ax_object_cache_impl = static_cast<AXObjectCacheImpl*>(ax_object_cache);
  const Position& parent_anchored_position = position.ToOffsetInAnchor();
  const Node* anchor_node = parent_anchored_position.AnchorNode();
  DCHECK(anchor_node);
  const AXObject* container = ax_object_cache_impl->GetOrCreate(anchor_node);
  DCHECK(container);
  if (anchor_node->IsTextNode()) {
    // Convert from a DOM offset that may have uncompressed white space to a
    // character offset.
    const auto first_position = Position::FirstPositionInNode(*anchor_node);
    const EphemeralRange range(first_position, parent_anchored_position);
    int offset = TextIterator::RangeLength(range);
    AXPosition ax_position(*container);
    ax_position.offset_ = offset;
    return ax_position;
  }

  const Node* node_after_position = position.ComputeNodeAfterPosition();
  const AXObject* ax_child =
      ax_object_cache_impl->GetOrCreate(node_after_position);
  DCHECK(ax_child);
  AXPosition ax_position(*container);
  if (ax_child->IsDescendantOf(*container))
    ax_position.child_index_ = ax_child->IndexInParent();
}
else {
  ax_position.child_index_ = static_cast<int>(container->Children().size());
}
return ax_position;
}

// Only for use by |AXRange| to represent empty selection ranges.
AXPosition::AXPosition()
    : container_object_(nullptr),
      child_index_(),
      offset_(),
      affinity_(TextAffinity::kDownstream) {
#if DCHECK_IS_ON()
  dom_tree_version_ = 0;
  style_version_ = 0;
#endif
}

AXPosition::AXPosition(const AXObject& container)
    : container_object_(&container),
      child_index_(),
      offset_(),
      affinity_(TextAffinity::kDownstream) {
  const Document* document = container_object_->GetDocument();
  DCHECK(document);
#if DCHECK_IS_ON()
  dom_tree_version_ = document->DomTreeVersion();
  style_version_ = document->StyleVersion();
#endif
  DCHECK(IsValid());
}

int AXPosition::ChildIndex() const {
  if (!IsTextPosition())
    return *child_index_;
  NOTREACHED();
}

int AXPosition::Offset() const {
  if (IsTextPosition())
    return *offset_;
  NOTREACHED();
}

bool AXPosition::IsValid() const {
  if (!container_object_ || container_object_->IsDetached())
    return false;
  if (!container_object_->GetNode() ||
      !container_object_->GetNode()->isConnected())
    return false;

  // We can't have both an object and a text anchored position, but we must have
  // at least one of them.
  DCHECK(!child_index_ || !offset_);
  DCHECK(child_index_ || offset_);
  if (offset_ && !container_object_->GetNode()->IsTextNode())
    return false;
  if (child_index_ &&
      child_index_ >= static_cast<int>(container_object_->Children().size())) {
    return false;
  }
  const AXObject* ax_child =
      *(container_object_->Children().begin() + child_index_);
  if (!ax_child->GetNode() || !ax_child->GetNode()->isConnected())
    return false;

  DCHECK(!container_object_->GetNode()->GetDocument().NeedsLayoutTreeUpdate());
  DCHECK_EQ(container_object_->GetNode()->GetDocument().DomTreeVersion(),
            dom_tree_version_);
  DCHECK_EQ(container_object_->GetNode()->GetDocument().StyleVersion(),
            style_version_);
  return true;
}

bool AXPosition::IsTextPosition() const {
  return IsValid() && offset_;
}

const PositionWithAffinity AXPosition::ToPositionWithAffinity() const {
  if (!IsValid())
    return {};

  const Node* container_node = container_object_->GetNode();
  if (!IsTextPosition()) {
    if (child_index_ ==
        static_cast<int>(container_object_->Children().size())) {
      return PositionWithAffinity(Position::LastPositionInNode(*container_node),
                                  affinity_);
    }
    const AXObject* ax_child =
        *(container_object_->Children().begin() + child_index_);
    return PositionWithAffinity(
        Position::InParentBeforeNode(*(ax_child->GetNode())), affinity_);
  }

  const auto first_position = Position::FirstPositionInNode(*container_node);
  const auto last_position = Position::LastPositionInNode(*container_node);
  CharacterIterator character_iterator(first_position, last_position);
  const EphemeralRange range =
      character_iterator.CalculateCharacterSubrange(0, *offset_);
  return PositionWithAffinity(range.EndPosition(), affinity_);
}

}  // namespace blink
