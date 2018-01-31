/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/editing/VisibleUnits.h"

#include "core/editing/EditingUtilities.h"
#include "core/editing/EphemeralRange.h"
#include "core/editing/TextOffsetMapping.h"
#include "core/editing/VisiblePosition.h"
#include "core/layout/LayoutBlock.h"
#include "platform/instrumentation/tracing/TraceEvent.h"
#include "platform/text/TextBoundaries.h"

namespace blink {

EphemeralRangeInFlatTree ComputeWordAroundPosition(
    const PositionInFlatTree& position,
    AppendTrailingWhitespace append_trailing_whitespace) {
  if (position.IsNull())
    return EphemeralRangeInFlatTree();
  const TextOffsetMapping mapping(position);
  const String text = mapping.GetText();
  const auto& start_end_pair = FindWordForward(
      text.Characters16(), text.length(), mapping.ComputeTextOffset(position));
  const int start = start_end_pair.first;
  const int end = start_end_pair.second;
  if (start == end)
    return EphemeralRangeInFlatTree(mapping.MapToFirstPosition(text.length()));
  if (append_trailing_whitespace == AppendTrailingWhitespace::kDontAppend)
    return mapping.ComputeRange(start, end);
  return mapping.ComputeRange(start, mapping.SkipWhitespace(end));
}

PositionInFlatTree EndOfWordPosition(const PositionInFlatTree& position) {
  if (position.IsNull())
    return PositionInFlatTree();
  const TextOffsetMapping mapping(position);
  const String text = mapping.GetText();
  const int offset = mapping.ComputeTextOffset(position);
  const auto& start_end_pair =
      FindWordForward(text.Characters16(), text.length(), offset);
  const int start = start_end_pair.first;
  const int end = start_end_pair.second;
  if (start == end)
    return mapping.MapToFirstPosition(end);
  return mapping.MapToLastPosition(end - 1);
}

PositionInFlatTree EndOfWordPositionBackward(
    const PositionInFlatTree& position) {
  if (position.IsNull())
    return PositionInFlatTree();
  const TextOffsetMapping mapping(position);
  const String text = mapping.GetText();
  const int offset = mapping.ComputeTextOffset(position);
  const auto& start_end_pair =
      FindWordBackward(text.Characters16(), text.length(), offset);
  const int start = start_end_pair.first;
  const int end = start_end_pair.second;
  if (start == end)
    return mapping.MapToFirstPosition(end);
  return mapping.MapToLastPosition(end - 1);
}

PositionInFlatTree EndOfWordPosition(const PositionInFlatTree& position,
                                     EWordSide side) {
  if (side == kPreviousWordIfOnBoundary)
    return EndOfWordPositionBackward(position);
  return EndOfWordPosition(position);
}

Position EndOfWordPosition(const VisiblePosition& position, EWordSide side) {
  return ToPositionInDOMTree(
      EndOfWordPosition(ToPositionInFlatTree(position.DeepEquivalent()), side));
}

VisiblePosition EndOfWord(const VisiblePosition& position, EWordSide side) {
  return CreateVisiblePosition(EndOfWordPosition(position, side),
                               TextAffinity::kUpstreamIfPossible);
}

PositionInFlatTree EndOfWordPosition(const VisiblePositionInFlatTree& position,
                                     EWordSide side) {
  return EndOfWordPosition(position.DeepEquivalent(), side);
}

VisiblePositionInFlatTree EndOfWord(const VisiblePositionInFlatTree& position,
                                    EWordSide side) {
  return CreateVisiblePosition(EndOfWordPosition(position, side),
                               TextAffinity::kUpstreamIfPossible);
}

// ----
static PositionInFlatTree FirstPositionInNextBlock(
    const PositionInFlatTree& position) {
  DCHECK(position.IsNotNull());
  const Node& container = *position.ComputeContainerNode();
  LayoutBlock& block = *container.GetLayoutObject()->ContainingBlock();
  for (LayoutObject* runner = block.NextInPreOrderAfterChildren(); runner;
       runner = runner->NextInPreOrder()) {
    if (runner->IsLayoutBlock())
      return ComputeStartPosition(ToLayoutBlock(*runner));
  }
  return PositionInFlatTree();
}

static PositionInFlatTree NextWordPosition(
    const PositionInFlatTree& passed_position) {
  PositionInFlatTree block_end_position;
  for (PositionInFlatTree position = passed_position; position.IsNotNull();
       position = FirstPositionInNextBlock(position)) {
    const TextOffsetMapping mapping(position);
    const String text = mapping.GetText();
    const int offset = mapping.ComputeTextOffset(position);
    const bool kForward = true;
    const int word_end = FindNextWordFromIndex(text.Characters16(),
                                               text.length(), offset, kForward);
    if (offset != word_end)
      return mapping.MapToFirstPosition(word_end);
    block_end_position = mapping.GetRange().EndPosition();
  }
  DCHECK(block_end_position.IsNotNull());
  return block_end_position;
}

VisiblePosition NextWordPosition(const VisiblePosition& position_in_dom) {
  DCHECK(position_in_dom.IsValid()) << position_in_dom;
  const PositionInFlatTree position =
      NextWordPosition(ToPositionInFlatTree(position_in_dom.DeepEquivalent()));
  return CreateVisiblePosition(HonorEditingBoundaryAtOrAfter(
      PositionWithAffinity(ToPositionInDOMTree(position),
                           TextAffinity::kUpstreamIfPossible),
      position_in_dom.DeepEquivalent()));
}

// ----
static PositionInFlatTree FirstPositionInPreviousBlock(
    const PositionInFlatTree& position) {
  DCHECK(position.IsNotNull());
  const Node& container = *position.ComputeContainerNode();
  LayoutBlock& block = *container.GetLayoutObject()->ContainingBlock();
  for (LayoutObject* runner = block.PreviousInPreOrder(); runner;
       runner = runner->PreviousInPreOrder()) {
    if (runner->IsLayoutBlock())
      return ComputeStartPosition(ToLayoutBlock(*runner));
  }
  return PositionInFlatTree();
}

static PositionInFlatTree PreviousWordPosition(
    const PositionInFlatTree& passed_position) {
  for (PositionInFlatTree position = passed_position; position.IsNotNull();
       position = FirstPositionInPreviousBlock(position)) {
    const TextOffsetMapping mapping(position);
    const String text = mapping.GetText();
    const int offset = mapping.ComputeTextOffset(position);
    const bool kBackward = false;
    const int word_start = FindNextWordFromIndex(
        text.Characters16(), text.length(), offset, kBackward);
    if (offset > 0 || word_start == 0)
      return mapping.MapToFirstPosition(word_start);
  }
  return PositionInFlatTree();
}

VisiblePosition PreviousWordPosition(const VisiblePosition& position_in_dom) {
  DCHECK(position_in_dom.IsValid()) << position_in_dom;
  const PositionInFlatTree position = PreviousWordPosition(
      ToPositionInFlatTree(position_in_dom.DeepEquivalent()));
  return CreateVisiblePosition(HonorEditingBoundaryAtOrBefore(
      PositionWithAffinity(ToPositionInDOMTree(position)),
      position_in_dom.DeepEquivalent()));
}

// ----
PositionInFlatTree StartOfWordPosition(const PositionInFlatTree& position) {
  if (position.IsNull())
    return PositionInFlatTree();
  const TextOffsetMapping mapping(position);
  const String text = mapping.GetText();
  const int offset = mapping.ComputeTextOffset(position);
  const auto& start_end_pair =
      FindWordForward(text.Characters16(), text.length(), offset);
  const int end = start_end_pair.second;
  if (offset != end)
    return mapping.MapToFirstPosition(start_end_pair.first);
  // |position| is at end of word, e.g. "abc| def", try to find following word.
  // "abc| def" -> "abc |def"
  if (static_cast<unsigned>(end) == text.length())
    return mapping.MapToLastPosition(end);
  const auto& start_end_pair2 =
      FindWordForward(text.Characters16(), text.length(), end);
  return mapping.MapToFirstPosition(start_end_pair2.first);
}

PositionInFlatTree StartOfWordPositionBackward(
    const PositionInFlatTree& position) {
  if (position.IsNull())
    return PositionInFlatTree();
  const TextOffsetMapping mapping(position);
  const String text = mapping.GetText();
  const int offset = mapping.ComputeTextOffset(position);
  const auto& start_end_pair =
      FindWordBackward(text.Characters16(), text.length(), offset);
  const int start = start_end_pair.first;
  return mapping.MapToFirstPosition(start);
}

PositionInFlatTree StartOfWordPosition(const PositionInFlatTree& position,
                                       EWordSide side) {
  if (side == kPreviousWordIfOnBoundary)
    return StartOfWordPositionBackward(position);
  return StartOfWordPosition(position);
}

Position StartOfWordPosition(const VisiblePosition& position, EWordSide side) {
  return ToPositionInDOMTree(StartOfWordPosition(
      ToPositionInFlatTree(position.DeepEquivalent()), side));
}

VisiblePosition StartOfWord(const VisiblePosition& position, EWordSide side) {
  return CreateVisiblePosition(StartOfWordPosition(position, side));
}

PositionInFlatTree StartOfWordPosition(
    const VisiblePositionInFlatTree& position,
    EWordSide side) {
  return StartOfWordPosition(position.DeepEquivalent(), side);
}

VisiblePositionInFlatTree StartOfWord(const VisiblePositionInFlatTree& position,
                                      EWordSide side) {
  return CreateVisiblePosition(StartOfWordPosition(position, side));
}

}  // namespace blink
