// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/TextOffsetMapping.h"

#include "core/dom/Node.h"
#include "core/editing/EditingUtilities.h"
#include "core/editing/Position.h"
#include "core/editing/iterators/CharacterIterator.h"
#include "core/editing/iterators/TextIterator.h"
#include "core/layout/LayoutBlock.h"

namespace blink {

PositionInFlatTree ComputeEndPosition(const LayoutBlock& block) {
  for (LayoutObject* runner = block.LastLeafChild(); runner;
       runner = runner->PreviousInPreOrder()) {
    Node* node = runner->NonPseudoNode();
    if (!node)
      continue;
    if (node->IsTextNode())
      return PositionInFlatTree(node, ToText(node)->length());
    return PositionInFlatTree::AfterNode(*node);
  }
  return PositionInFlatTree();
}

PositionInFlatTree ComputeStartPosition(const LayoutBlock& block) {
  for (LayoutObject* runner = block.FirstChild(); runner;
       runner = runner->NextInPreOrder()) {
    Node* node = runner->NonPseudoNode();
    if (!node)
      continue;
    if (node->IsTextNode())
      return PositionInFlatTree(node, 0);
    return PositionInFlatTree::BeforeNode(*node);
  }
  return PositionInFlatTree();
}

namespace {

// Returns range of block containing |position|.
// Note: Container node of |position| should be associated to |LayoutObject|.
EphemeralRangeInFlatTree ComputeBlockRange(const PositionInFlatTree& position) {
  DCHECK(position.IsNotNull());
  const Node& container = *position.ComputeContainerNode();
  LayoutBlock& block = *container.GetLayoutObject()->ContainingBlock();
  const PositionInFlatTree& start = ComputeStartPosition(block);
  const PositionInFlatTree& end = ComputeEndPosition(block);
  return EphemeralRangeInFlatTree(start, end);
}

String Ensure16Bit(const String& text) {
  String text16(text);
  text16.Ensure16Bit();
  return text16;
}

}  // namespace

TextOffsetMapping::TextOffsetMapping(const PositionInFlatTree& position,
                                     const TextIteratorBehavior behavior)
    : behavior_(behavior),
      range_(ComputeBlockRange(position)),
      text16_(Ensure16Bit(PlainText(range_, behavior_))) {}

TextOffsetMapping::TextOffsetMapping(const PositionInFlatTree& position)
    : TextOffsetMapping(position,
                        TextIteratorBehavior::Builder()
                            .SetEmitsCharactersBetweenAllVisiblePositions(true)
                            //.SetEmitsObjectReplacementCharacter(true)
                            .SetEmitsSmallXForTextSecurity(true)
                            .Build()) {}

PositionInFlatTree TextOffsetMapping::MapToFirstPosition(
    unsigned offset) const {
  DCHECK_LE(offset, text16_.length());
  CharacterIteratorInFlatTree iterator(range_, behavior_);
  if (offset >= 1 && offset == text16_.length()) {
    iterator.Advance(offset - 1);
    return iterator.ComputeLastPosition();
  }
  iterator.Advance(offset);
  return iterator.ComputeFirstPosition();
}

PositionInFlatTree TextOffsetMapping::MapToLastPosition(unsigned offset) const {
  DCHECK_LE(offset, text16_.length());
  CharacterIteratorInFlatTree iterator(range_, behavior_);
  if (offset >= 1 && offset == text16_.length()) {
    iterator.Advance(offset - 1);
    return iterator.ComputeLastPosition();
  }
  iterator.Advance(offset);
  return iterator.ComputeLastPosition();
}

EphemeralRangeInFlatTree TextOffsetMapping::ComputeRange(unsigned start,
                                                         unsigned end) const {
  DCHECK_LE(end, text16_.length());
  DCHECK_LE(start, end);
  if (start == end)
    return EphemeralRangeInFlatTree();
  return EphemeralRangeInFlatTree(MapToFirstPosition(start),
                                  MapToLastPosition(end - 1));
}

int TextOffsetMapping::ComputeTextOffset(
    const PositionInFlatTree& position) const {
  return TextIteratorInFlatTree::RangeLength(range_.StartPosition(), position,
                                             behavior_);
}

unsigned TextOffsetMapping::SkipWhitespace(unsigned offset) const {
  for (unsigned runner = offset; runner < text16_.length(); ++runner) {
    if (!IsWhitespace(text16_[runner]))
      return runner;
  }
  return text16_.length();
}

}  // namespace blink
