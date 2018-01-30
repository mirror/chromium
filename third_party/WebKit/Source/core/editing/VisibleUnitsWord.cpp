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
#include "core/editing/VisiblePosition.h"
#include "platform/instrumentation/tracing/TraceEvent.h"
#include "platform/text/TextBoundaries.h"

namespace blink {

namespace {

unsigned EndWordBoundary(
    const UChar* characters,
    unsigned length,
    unsigned offset,
    BoundarySearchContextAvailability may_have_more_context,
    bool& need_more_context) {
  DCHECK_LE(offset, length);
  if (may_have_more_context &&
      EndOfFirstWordBoundaryContext(characters + offset, length - offset) ==
          static_cast<int>(length - offset)) {
    need_more_context = true;
    return length;
  }
  need_more_context = false;
  return FindWordEndBoundary(characters, length, offset);
}

template <typename Strategy>
PositionTemplate<Strategy> EndOfWordAlgorithm(
    const VisiblePositionTemplate<Strategy>& position) {
  DCHECK(position.IsValid()) << position;
  if (IsEndOfParagraph(position))
    return position.DeepEquivalent();
  return NextBoundary(position, EndWordBoundary);
}

template <typename Strategy>
PositionTemplate<Strategy> EndOfWordBackwardAlgorithm(
    const VisiblePositionTemplate<Strategy>& position) {
  DCHECK(position.IsValid()) << position;
  if (IsStartOfParagraph(position))
    return position.DeepEquivalent();
  const VisiblePositionTemplate<Strategy>& previous_position =
      PreviousPositionOf(position);
  if (previous_position.IsNull())
    return position.DeepEquivalent();
  return NextBoundary(previous_position, EndWordBoundary);
}

unsigned NextWordPositionBoundary(
    const UChar* characters,
    unsigned length,
    unsigned offset,
    BoundarySearchContextAvailability may_have_more_context,
    bool& need_more_context) {
  if (may_have_more_context &&
      EndOfFirstWordBoundaryContext(characters + offset, length - offset) ==
          static_cast<int>(length - offset)) {
    need_more_context = true;
    return length;
  }
  need_more_context = false;
  return FindNextWordFromIndex(characters, length, offset, true);
}

unsigned PreviousWordPositionBoundary(
    const UChar* characters,
    unsigned length,
    unsigned offset,
    BoundarySearchContextAvailability may_have_more_context,
    bool& need_more_context) {
  if (may_have_more_context &&
      !StartOfLastWordBoundaryContext(characters, offset)) {
    need_more_context = true;
    return 0;
  }
  need_more_context = false;
  return FindNextWordFromIndex(characters, length, offset, false);
}

unsigned StartWordBoundary(
    const UChar* characters,
    unsigned length,
    unsigned offset,
    BoundarySearchContextAvailability may_have_more_context,
    bool& need_more_context) {
  TRACE_EVENT0("blink", "startWordBoundary");
  DCHECK(offset);
  if (may_have_more_context &&
      !StartOfLastWordBoundaryContext(characters, offset)) {
    need_more_context = true;
    return 0;
  }
  need_more_context = false;
  U16_BACK_1(characters, 0, offset);
  return FindWordStartBoundary(characters, length, offset);
}

template <typename Strategy>
PositionTemplate<Strategy> StartOfWordAlgorithm(
    const VisiblePositionTemplate<Strategy>& position) {
  DCHECK(position.IsValid()) << position;
  if (IsEndOfParagraph(position))
    return position.DeepEquivalent();
  const VisiblePositionTemplate<Strategy>& next_position =
      NextPositionOf(position);
  if (next_position.IsNull())
    return position.DeepEquivalent();
  return PreviousBoundary(next_position, StartWordBoundary);
}

template <typename Strategy>
PositionTemplate<Strategy> StartOfWordBackwardAlgorithm(
    const VisiblePositionTemplate<Strategy>& position) {
  DCHECK(position.IsValid()) << position;
  // TODO(yosin) This returns a null VP for c at the start of the document at
  // paragraph end, the startofWord is the current position
  return PreviousBoundary(position, StartWordBoundary);
}

}  // namespace

Position EndOfWordPosition(const VisiblePosition& position) {
  return EndOfWordAlgorithm<EditingStrategy>(position);
}

Position EndOfWordPositionBackward(const VisiblePosition& position) {
  return EndOfWordBackwardAlgorithm<EditingStrategy>(position);
}

VisiblePosition EndOfWord(const VisiblePosition& position) {
  return CreateVisiblePosition(EndOfWordPosition(position),
                               TextAffinity::kUpstreamIfPossible);
}

VisiblePosition EndOfWordBackward(const VisiblePosition& position) {
  return CreateVisiblePosition(EndOfWordPositionBackward(position),
                               TextAffinity::kUpstreamIfPossible);
}

PositionInFlatTree EndOfWordPosition(
    const VisiblePositionInFlatTree& position) {
  return EndOfWordAlgorithm<EditingInFlatTreeStrategy>(position);
}

PositionInFlatTree EndOfWordPositionBackward(
    const VisiblePositionInFlatTree& position) {
  return EndOfWordBackwardAlgorithm<EditingInFlatTreeStrategy>(position);
}

VisiblePositionInFlatTree EndOfWord(const VisiblePositionInFlatTree& position) {
  return CreateVisiblePosition(EndOfWordPosition(position),
                               TextAffinity::kUpstreamIfPossible);
}

VisiblePositionInFlatTree EndOfWordBackward(
    const VisiblePositionInFlatTree& position) {
  return CreateVisiblePosition(EndOfWordPositionBackward(position),
                               TextAffinity::kUpstreamIfPossible);
}

VisiblePosition NextWordPosition(const VisiblePosition& c) {
  DCHECK(c.IsValid()) << c;
  VisiblePosition next =
      CreateVisiblePosition(NextBoundary(c, NextWordPositionBoundary),
                            TextAffinity::kUpstreamIfPossible);
  return HonorEditingBoundaryAtOrAfter(next, c.DeepEquivalent());
}

VisiblePosition PreviousWordPosition(const VisiblePosition& c) {
  DCHECK(c.IsValid()) << c;
  VisiblePosition prev =
      CreateVisiblePosition(PreviousBoundary(c, PreviousWordPositionBoundary));
  return HonorEditingBoundaryAtOrBefore(prev, c.DeepEquivalent());
}

Position StartOfWordPosition(const VisiblePosition& position) {
  return StartOfWordAlgorithm<EditingStrategy>(position);
}

Position StartOfWordPositionBackward(const VisiblePosition& position) {
  return StartOfWordBackwardAlgorithm<EditingStrategy>(position);
}

VisiblePosition StartOfWord(const VisiblePosition& position) {
  return CreateVisiblePosition(StartOfWordPosition(position));
}

VisiblePosition StartOfWordBackward(const VisiblePosition& position) {
  return CreateVisiblePosition(StartOfWordPositionBackward(position));
}

PositionInFlatTree StartOfWordPosition(
    const VisiblePositionInFlatTree& position) {
  return StartOfWordAlgorithm<EditingInFlatTreeStrategy>(position);
}

PositionInFlatTree StartOfWordPositionBackward(
    const VisiblePositionInFlatTree& position) {
  return StartOfWordBackwardAlgorithm<EditingInFlatTreeStrategy>(position);
}

VisiblePositionInFlatTree StartOfWord(
    const VisiblePositionInFlatTree& position) {
  return CreateVisiblePosition(StartOfWordPosition(position));
}

VisiblePositionInFlatTree StartOfWordBackward(
    const VisiblePositionInFlatTree& position) {
  return CreateVisiblePosition(StartOfWordPositionBackward(position));
}

}  // namespace blink
