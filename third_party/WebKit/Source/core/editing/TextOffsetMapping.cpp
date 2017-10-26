// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/TextOffsetMapping.h"

#include "core/dom/FlatTreeTraversal.h"
#include "core/dom/Node.h"
#include "core/editing/EphemeralRange.h"
#include "core/editing/Position.h"
#include "core/editing/iterators/CharacterIterator.h"
#include "core/editing/iterators/TextIterator.h"
#include "core/layout/LayoutObject.h"

namespace blink {

namespace {

Node* EnclosingBlockFlowOf(const Node& node) {
  for (Node& runner : FlatTreeTraversal::InclusiveAncestorsOf(node)) {
    LayoutObject* const layout_object = runner.GetLayoutObject();
    if (layout_object->IsLayoutBlockFlow())
      return &runner;
  }
  return nullptr;
}

Node* EnclosingBlockFlowOf(const PositionInFlatTree& position) {
  DCHECK(position.IsNotNull());
  const Node& container = *position.ComputeContainerNode();
  const Node* block = EnclosingBlockFlowOf(container);
  if (!block)
    return nullptr;
  return EnclosingBlockFlowOf(*block);
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
      block_(EnclosingBlockFlowOf(position)),
      text16_(Ensure16Bit(PlainText(GetRange(), behavior_))) {}

TextOffsetMapping::TextOffsetMapping(const PositionInFlatTree& position)
    : TextOffsetMapping(position,
                        TextIteratorBehavior::Builder()
                            .SetEmitsObjectReplacementCharacter(true)
                            .Build()) {}

PositionInFlatTree TextOffsetMapping::ComputeFirstPosition(
    unsigned offset) const {
  DCHECK_LE(offset, text16_.length());
  CharacterIteratorInFlatTree iterator(GetRange(), behavior_);
  iterator.Advance(offset);
  return iterator.StartPosition();
}

PositionInFlatTree TextOffsetMapping::ComputeLastPosition(
    unsigned offset) const {
  DCHECK_LE(offset, text16_.length());
  CharacterIteratorInFlatTree iterator(GetRange(), behavior_);
  iterator.Advance(offset);
  return iterator.EndPosition();
}

EphemeralRangeInFlatTree TextOffsetMapping::ComputeRange(unsigned start,
                                                         unsigned end) const {
  DCHECK_LE(start, end);
  if (start == end)
    return EphemeralRangeInFlatTree();
  return EphemeralRangeInFlatTree(ComputeFirstPosition(start),
                                  ComputeLastPosition(end));
}

int TextOffsetMapping::ComputeTextOffset(
    const PositionInFlatTree& position) const {
  return TextIteratorInFlatTree::RangeLength(GetRange().StartPosition(),
                                             position, behavior_);
}

EphemeralRangeInFlatTree TextOffsetMapping::GetRange() const {
  if (!block_)
    return {};
  return EphemeralRangeInFlatTree::RangeOfContents(*block_);
}

}  // namespace blink
