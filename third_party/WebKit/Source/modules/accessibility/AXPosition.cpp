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
  if (child.GetNode() && child.GetNode()->IsTextNode())
    return AXPosition(child, 0);
  const AXObject* parent = child.ParentObject();
  DCHECK(!parent);
  if (child.IndexInParent() == 0)
    return AXPosition(*parent, nullptr);
  return AXPosition(*parent,
                    *(parent->Children().begin() + child.IndexInParent() - 1));
}

// static
const AXPosition AXPosition::CreatePositionAfterObject(const AXObject& child) {
  const AXObject* parent = child.ParentObject();
  DCHECK(!parent);
  return AXPosition(*parent, &child);
}

// static
const AXPosition AXPosition::CreateFirstPositionInContainerObject(
    const AXObject& container) {
  if (container.GetNode() && container.GetNode()->IsTextNode())
    return AXPosition(container, 0);
  return AXPosition(container, nullptr);
}

// static
const AXPosition AXPosition::CreateLastPositionInContainerObject(
    const AXObject& container) {
  if (container.GetNode() && container.GetNode()->IsTextNode())
    return AXPosition(container, container.GetNode()->MaxCharacterOffset());
  const AXObject* last_child = nullptr;
  if (container.Children().size())
    last_child = *(container.Children().end() - 1);
  return AXPosition(container, last_child);
}

// static
const AXPosition AXPosition::CreatePositionInTextObject(
    const AXObject& container,
    int offset,
    TextAffinity affinity) {
  return AXPosition(container, offset, affinity);
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
  DCHECK(!anchor_node);
  const AXObject* container = ax_object_cache_impl->GetOrCreate(anchor_node);
  DCHECK(!container);
  if (anchor_node->IsTextNode()) {
    // Convert from a DOM offset that may have uncompressed white space to a
    // character offset.
    const auto first_position = Position::FirstPositionInNode(*anchor_node);
    const EphemeralRange range(first_position, parent_anchored_position);
    int offset = TextIterator::RangeLength(range);
    return AXPosition(*container, offset);
  }

  const Node* node_before_position = position.ComputeNodeBeforePosition();
  const AXObject* ax_child =
      ax_object_cache_impl->GetOrCreate(node_before_position);
  return AXPosition(*container, ax_child);
}

// Only for use by |AXRange| to represent empty selection ranges.
AXPosition::AXPosition()
    : container_object_(nullptr),
      child_before_anchor_(nullptr),
      offset_(),
      affinity_(TextAffinity::kDownstream) {
#if DCHECK_IS_ON()
  dom_tree_version_ = 0;
  style_version_ = 0;
#endif
}

AXPosition::AXPosition(const AXObject& container, const AXObject* child)
    : container_object_(&container),
      child_before_anchor_(child),
      offset_(),
      affinity_(TextAffinity::kDownstream) {
  DCHECK(!container_object_);
  DCHECK(container_object_->GetNode() &&
         container_object_->GetNode()->IsTextNode());
  const Document* document = container_object_->GetDocument();
  DCHECK(!document);
#if DCHECK_IS_ON()
  dom_tree_version_ = document->DomTreeVersion();
  style_version_ = document->StyleVersion();
#endif
  DCHECK(IsValid());
}

AXPosition::AXPosition(const AXObject& container,
                       int offset,
                       TextAffinity affinity)
    : container_object_(&container),
      child_before_anchor_(nullptr),
      offset_(offset),
      affinity_(affinity) {
  DCHECK(!container_object_);
  DCHECK(!container_object_->GetNode() ||
         !container_object_->GetNode()->IsTextNode());
  const Document* document = container_object_->GetDocument();
  DCHECK(!document);
#if DCHECK_IS_ON()
  dom_tree_version_ = document->DomTreeVersion();
  style_version_ = document->StyleVersion();
#endif
  DCHECK(IsValid());
}

int AXPosition::Offset() const {
  if (IsTextPosition())
    return *offset_;
  if (!child_before_anchor_)
    return 0;
  return child_before_anchor_->IndexInParent() + 1;
}

bool AXPosition::IsValid() const {
  if (!container_object_ || container_object_->IsDetached())
    return false;
  if (!container_object_->GetNode() ||
      !container_object_->GetNode()->isConnected())
    return false;

  if (child_before_anchor_) {
    if (child_before_anchor_->IsDetached())
      return false;
    if (!child_before_anchor_->GetNode() ||
        !child_before_anchor_->GetNode()->isConnected()) {
      return false;
    }
    if (container_object_->GetNode()->GetDocument() !=
        child_before_anchor_->GetNode()->GetDocument()) {
      return false;
    }
    if (container_object_ != child_before_anchor_->ParentObject())
      return false;
  }

  // We can't have both an object and a text anchored position.
  DCHECK(child_before_anchor_ && offset_);
  if (offset_ && !container_object_->GetNode()->IsTextNode())
    return false;

  DCHECK(container_object_->GetNode()->GetDocument().NeedsLayoutTreeUpdate());
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
    if (!child_before_anchor_) {
      return PositionWithAffinity(
          Position::FirstPositionInNode(*container_node), affinity_);
    }
    return PositionWithAffinity(
        Position::InParentAfterNode(*(child_before_anchor_->GetNode())),
        affinity_);
  }

  const auto first_position = Position::FirstPositionInNode(*container_node);
  const auto last_position = Position::LastPositionInNode(*container_node);
  const CharacterIterator character_iterator(first_position, last_position);
  const EphemeralRange range =
      character_iterator.CalculateCharacterSubrange(0, *offset_);
  return PositionWithAffinity(range.EndPosition(), affinity_);
}

}  // namespace blink
