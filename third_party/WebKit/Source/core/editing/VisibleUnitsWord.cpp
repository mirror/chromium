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
#include "platform/instrumentation/tracing/TraceEvent.h"
#include "platform/text/TextBoundaries.h"

namespace blink {

EphemeralRangeInFlatTree ComputeWordAroundPosition(
    const PositionInFlatTree& position,
    AppendTrailingWhitespace append_trailing_whitespace) {
  if (position.IsNull())
    return {};
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

static PositionInFlatTree EndOfWordPosition(const PositionInFlatTree& position,
                                            EWordSide side) {
  // TODO(editing-dev): Support |side| in |EndOfWordPosition()|
  const EphemeralRangeInFlatTree range = ComputeWordAroundPosition(
      position, AppendTrailingWhitespace::kDontAppend);
  DCHECK(range.IsNotNull()) << position;
  return range.EndPosition();
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

static PositionInFlatTree NextWordPosition(const PositionInFlatTree& position) {
  const TextOffsetMapping mapping(position);
  const String text = mapping.GetText();
  const auto start_end_pair = FindWordForward(
      text.Characters16(), text.length(), mapping.ComputeTextOffset(position));
  const int start = start_end_pair.first;
  const int end = start_end_pair.second;
  if (start == end) {
    return mapping.MapToFirstPosition(start);
  }
  const int offset = mapping.ComputeTextOffset(position);
  if (offset == end) {
    DCHECK_EQ(static_cast<unsigned>(offset), text.length());
    NOTREACHED();
    // TODO(yosin): Go to next block
    return position;
  }
  return mapping.MapToFirstPosition(end);
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

static PositionInFlatTree PreviousWordPosition(
    const PositionInFlatTree& position) {
  const TextOffsetMapping mapping(position);
  const String text = mapping.GetText();
  const auto start_end_pair = FindWordBackward(
      text.Characters16(), text.length(), mapping.ComputeTextOffset(position));
  const int start = start_end_pair.first;
  const int end = start_end_pair.second;
  if (start == end) {
    return mapping.MapToFirstPosition(start);
  }
  const int offset = mapping.ComputeTextOffset(position);
  if (offset == start) {
    DCHECK_EQ(offset, 0);
    NOTREACHED();
    // TODO(yosin): Go to previous block
    return position;
  }
  return mapping.MapToFirstPosition(end);
}

VisiblePosition PreviousWordPosition(const VisiblePosition& position_in_dom) {
  DCHECK(position_in_dom.IsValid()) << position_in_dom;
  const PositionInFlatTree position = PreviousWordPosition(
      ToPositionInFlatTree(position_in_dom.DeepEquivalent()));
  return CreateVisiblePosition(HonorEditingBoundaryAtOrBefore(
      PositionWithAffinity(ToPositionInDOMTree(position)),
      position_in_dom.DeepEquivalent()));
}

static PositionInFlatTree StartOfWordPosition(
    const PositionInFlatTree& position,
    EWordSide side) {
  // TODO(editing-dev): Support |side| in |EndOfWordPosition()|
  const EphemeralRangeInFlatTree range = ComputeWordAroundPosition(
      position, AppendTrailingWhitespace::kDontAppend);
  return range.StartPosition();
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
