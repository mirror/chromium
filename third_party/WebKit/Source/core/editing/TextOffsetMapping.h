// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TextOffsetMapping_h
#define TextOffsetMapping_h

#include "base/macros.h"
#include "core/CoreExport.h"
#include "core/editing/Forward.h"
#include "core/editing/iterators/TextIteratorBehavior.h"
#include "platform/heap/Member.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class Node;

// Mapping between position and text offset.
class TextOffsetMapping final {
  STACK_ALLOCATED();

 public:
  TextOffsetMapping(const PositionInFlatTree&, const TextIteratorBehavior);
  explicit TextOffsetMapping(const PositionInFlatTree& block);
  ~TextOffsetMapping() = default;

  int ComputeTextOffset(const PositionInFlatTree&) const;
  PositionInFlatTree ComputeFirstPosition(unsigned offset) const;
  PositionInFlatTree ComputeLastPosition(unsigned offset) const;
  EphemeralRangeInFlatTree ComputeRange(unsigned start, unsigned end) const;

  const String& GetText() const { return text16_; }

 private:
  EphemeralRangeInFlatTree GetRange() const;

  const TextIteratorBehavior behavior_;
  const Member<Node> block_;
  const String text16_;

  DISALLOW_COPY_AND_ASSIGN(TextOffsetMapping);
};

}  // namespace blink

#endif  // TextOffsetMapping_h
