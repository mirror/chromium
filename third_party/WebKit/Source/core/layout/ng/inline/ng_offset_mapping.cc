// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/inline/ng_offset_mapping.h"

#include "core/dom/Node.h"
#include "core/dom/Text.h"
#include "core/editing/EphemeralRange.h"
#include "core/editing/Position.h"
#include "core/layout/ng/inline/ng_inline_node.h"

namespace blink {

namespace {

// Offset mapping APIs accept only two types of positions:
// 1. Offset-in-anchor in a text node
// 2. Before/After-anchor to an atomic inline
void AssertValidPositionForOffsetMapping(const Position& position) {
#if DCHECK_IS_ON()
  DCHECK(position.IsNotNull());
  if (position.AnchorNode()->IsTextNode()) {
    DCHECK(position.IsOffsetInAnchor());
    return;
  }
  DCHECK(position.IsBeforeAnchor() || position.IsAfterAnchor());
  DCHECK(position.AnchorNode()->GetLayoutObject());
  DCHECK(position.AnchorNode()->GetLayoutObject()->IsAtomicInlineLevel());
#endif
}

Position CreatePositionForOffsetMapping(const Node& node, unsigned dom_offset) {
  if (node.IsTextNode())
    return Position(&node, dom_offset);
  // For non-text-anchored position, the offset must be either 0 or 1.
  DCHECK_LE(dom_offset, 1u);
  return dom_offset ? Position::AfterNode(node) : Position::BeforeNode(node);
}

unsigned GetOffsetForMapping(const Position& position) {
  AssertValidPositionForOffsetMapping(position);
  if (position.AnchorNode()->IsTextNode())
    return position.OffsetInContainerNode();
  if (position.IsBeforeAnchor())
    return 0;
  return 1;
}

}  // namespace

NGOffsetMappingUnit::NGOffsetMappingUnit(NGOffsetMappingUnitType type,
                                         const Node& node,
                                         unsigned dom_start,
                                         unsigned dom_end,
                                         unsigned text_content_start,
                                         unsigned text_content_end)
    : type_(type),
      owner_(&node),
      dom_start_(dom_start),
      dom_end_(dom_end),
      text_content_start_(text_content_start),
      text_content_end_(text_content_end) {}

NGOffsetMappingUnit::~NGOffsetMappingUnit() = default;

unsigned NGOffsetMappingUnit::ConvertDOMOffsetToTextContent(
    unsigned offset) const {
  DCHECK_GE(offset, dom_start_);
  DCHECK_LE(offset, dom_end_);
  // DOM start is always mapped to text content start.
  if (offset == dom_start_)
    return text_content_start_;
  // DOM end is always mapped to text content end.
  if (offset == dom_end_)
    return text_content_end_;
  // Handle collapsed mapping.
  if (text_content_start_ == text_content_end_)
    return text_content_start_;
  // Handle has identity mapping.
  return offset - dom_start_ + text_content_start_;
}

unsigned NGOffsetMappingUnit::ConvertTextContentToFirstDOMOffset(
    unsigned offset) const {
  DCHECK_GE(offset, text_content_start_);
  DCHECK_LE(offset, text_content_end_);
  // Always return DOM start for collapsed units.
  if (text_content_start_ == text_content_end_)
    return dom_start_;
  // Handle identity mapping.
  if (type_ == NGOffsetMappingUnitType::kIdentity)
    return dom_start_ + offset - text_content_start_;
  // Handle expanded mapping.
  return offset < text_content_end_ ? dom_start_ : dom_end_;
}

unsigned NGOffsetMappingUnit::ConvertTextContentToLastDOMOffset(
    unsigned offset) const {
  DCHECK_GE(offset, text_content_start_);
  DCHECK_LE(offset, text_content_end_);
  // Always return DOM end for collapsed units.
  if (text_content_start_ == text_content_end_)
    return dom_end_;
  // In a non-collapsed unit, mapping between DOM and text content offsets is
  // one-to-one. Reuse existing code.
  return ConvertTextContentToFirstDOMOffset(offset);
}

// static
const NGOffsetMapping* NGOffsetMapping::GetFor(const Position& position) {
  AssertValidPositionForOffsetMapping(position);
  const Node& node = *position.AnchorNode();
  unsigned offset = GetOffsetForMapping(position);
  const LayoutObject* layout_object = AssociatedLayoutObjectOf(node, offset);
  if (!layout_object || !layout_object->IsInline())
    return nullptr;
  LayoutBlockFlow* block_flow = layout_object->EnclosingNGBlockFlow();
  if (!block_flow)
    return nullptr;
  DCHECK(block_flow->ChildrenInline());
  return &NGInlineNode(block_flow).ComputeOffsetMappingIfNeeded();
}

NGOffsetMapping::NGOffsetMapping(NGOffsetMapping&& other)
    : NGOffsetMapping(std::move(other.units_),
                      std::move(other.ranges_),
                      other.text_) {}

NGOffsetMapping::NGOffsetMapping(UnitVector&& units,
                                 RangeMap&& ranges,
                                 String text)
    : units_(units), ranges_(ranges), text_(text) {}

NGOffsetMapping::~NGOffsetMapping() = default;

const NGOffsetMappingUnit* NGOffsetMapping::GetMappingUnitForPosition(
    const Position& position) const {
  AssertValidPositionForOffsetMapping(position);
  unsigned range_start;
  unsigned range_end;
  std::tie(range_start, range_end) = ranges_.at(position.AnchorNode());
  const unsigned offset = GetOffsetForMapping(position);
  if (range_start == range_end || units_[range_start].DOMStart() > offset)
    return nullptr;
  // Find the last unit where unit.dom_start <= offset
  const NGOffsetMappingUnit* unit = std::prev(std::upper_bound(
      units_.begin() + range_start, units_.begin() + range_end, offset,
      [](unsigned offset, const NGOffsetMappingUnit& unit) {
        return offset < unit.DOMStart();
      }));
  if (unit->DOMEnd() < offset)
    return nullptr;
  return unit;
}

NGMappingUnitRange NGOffsetMapping::GetMappingUnitsForDOMRange(
    const EphemeralRange& ephemeral_range) const {
  AssertValidPositionForOffsetMapping(ephemeral_range.StartPosition());
  AssertValidPositionForOffsetMapping(ephemeral_range.EndPosition());
  DCHECK_EQ(ephemeral_range.StartPosition().AnchorNode(), ephemeral_range.EndPosition().AnchorNode());
  const Node* node = ephemeral_range.StartPosition().AnchorNode();
  const unsigned start_offset = GetOffsetForMapping(ephemeral_range.StartPosition());
  const unsigned end_offset = GetOffsetForMapping(ephemeral_range.EndPosition());
  unsigned range_start;
  unsigned range_end;
  std::tie(range_start, range_end) = ranges_.at(node);
  if (range_start == range_end || units_[range_start].DOMStart() > end_offset ||
      units_[range_end - 1].DOMEnd() < start_offset)
    return {};

  // Find the first unit where unit.dom_end >= start_offset
  const NGOffsetMappingUnit* result_begin = std::lower_bound(
      units_.begin() + range_start, units_.begin() + range_end, start_offset,
      [](const NGOffsetMappingUnit& unit, unsigned offset) {
        return unit.DOMEnd() < offset;
      });

  // Find the next of the last unit where unit.dom_start <= end_offset
  const NGOffsetMappingUnit* result_end =
      std::upper_bound(result_begin, units_.begin() + range_end, end_offset,
                       [](unsigned offset, const NGOffsetMappingUnit& unit) {
                         return offset < unit.DOMStart();
                       });

  return {result_begin, result_end};
}

Optional<unsigned> NGOffsetMapping::GetTextContentOffset(
    const Position& position) const {
  AssertValidPositionForOffsetMapping(position);
  const NGOffsetMappingUnit* unit = GetMappingUnitForPosition(position);
  if (!unit)
    return WTF::nullopt;
  return unit->ConvertDOMOffsetToTextContent(GetOffsetForMapping(position));
}

Position NGOffsetMapping::StartOfNextNonCollapsedCharacter(
    const Position& position) const {
  AssertValidPositionForOffsetMapping(position);
  Optional<unsigned> text_content_offset = GetTextContentOffset(position);
  if (!text_content_offset)
    return Position();
  Position candidate = GetLastPosition(*text_content_offset);
  if (candidate.IsNull() || candidate.AnchorNode() != position.AnchorNode())
    return Position();
  return candidate;
}

Position NGOffsetMapping::EndOfLastNonCollapsedCharacter(
    const Position& position) const {
  AssertValidPositionForOffsetMapping(position);
  Optional<unsigned> text_content_offset = GetTextContentOffset(position);
  if (!text_content_offset)
    return Position();
  Position candidate = GetFirstPosition(*text_content_offset);
  if (candidate.IsNull() || candidate.AnchorNode() != position.AnchorNode())
    return Position();
  return candidate;
}

bool NGOffsetMapping::IsBeforeNonCollapsedCharacter(
    const Position& position) const {
  AssertValidPositionForOffsetMapping(position);
  return position == StartOfNextNonCollapsedCharacter(position);
}

bool NGOffsetMapping::IsAfterNonCollapsedCharacter(
    const Position& position) const {
  AssertValidPositionForOffsetMapping(position);
  return position == EndOfLastNonCollapsedCharacter(position);
}

Optional<UChar> NGOffsetMapping::GetCharacterBefore(
    const Position& position) const {
  AssertValidPositionForOffsetMapping(position);
  Optional<unsigned> text_content_offset = GetTextContentOffset(position);
  if (!text_content_offset || !*text_content_offset)
    return WTF::nullopt;
  return text_[*text_content_offset - 1];
}

Position NGOffsetMapping::GetFirstPosition(unsigned offset) const {
  // Find the first unit where |unit.TextContentEnd() >= offset|
  if (units_.IsEmpty() || units_.back().TextContentEnd() < offset)
    return {};
  const NGOffsetMappingUnit* result =
      std::lower_bound(units_.begin(), units_.end(), offset,
                       [](const NGOffsetMappingUnit& unit, unsigned offset) {
                         return unit.TextContentEnd() < offset;
                       });
  DCHECK_NE(result, units_.end());
  if (result->TextContentStart() > offset)
    return {};
  const Node& node = result->GetOwner();
  const unsigned dom_offset =
      result->ConvertTextContentToFirstDOMOffset(offset);
  return CreatePositionForOffsetMapping(node, dom_offset);
}

Position NGOffsetMapping::GetLastPosition(unsigned offset) const {
  // Find the last unit where |unit.TextContentStart() <= offset|
  if (units_.IsEmpty() || units_.front().TextContentStart() > offset)
    return {};
  const NGOffsetMappingUnit* result =
      std::upper_bound(units_.begin(), units_.end(), offset,
                       [](unsigned offset, const NGOffsetMappingUnit& unit) {
                         return offset < unit.TextContentStart();
                       });
  DCHECK_NE(result, units_.begin());
  result = std::prev(result);
  if (result->TextContentEnd() < offset)
    return {};
  const Node& node = result->GetOwner();
  const unsigned dom_offset = result->ConvertTextContentToLastDOMOffset(offset);
  return CreatePositionForOffsetMapping(node, dom_offset);
}

}  // namespace blink
