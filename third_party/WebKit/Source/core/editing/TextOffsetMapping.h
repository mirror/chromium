// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TextOffsetMapping_h
#define TextOffsetMapping_h

#include "base/macros.h"
#include "core/CoreExport.h"
#include "core/editing/EphemeralRange.h"
#include "core/editing/Forward.h"
#include "core/editing/iterators/TextIteratorBehavior.h"
#include "platform/heap/Member.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

class LayoutBlock;

// Mapping between position and text offset.
class CORE_EXPORT TextOffsetMapping final {
  STACK_ALLOCATED();

 public:
  explicit TextOffsetMapping(const PositionInFlatTree&);
  ~TextOffsetMapping() = default;

  const EphemeralRangeInFlatTree GetRange() const { return range_; }
  const String& GetText() const { return text16_; }

  int ComputeTextOffset(const PositionInFlatTree&) const;
  PositionInFlatTree MapToFirstPosition(unsigned offset) const;
  PositionInFlatTree MapToLastPosition(unsigned offset) const;
  EphemeralRangeInFlatTree ComputeRange(unsigned start, unsigned end) const;
  unsigned SkipWhitespace(unsigned) const;

 private:
  friend class TextOffsetMappingTest;

  TextOffsetMapping(const PositionInFlatTree&, const TextIteratorBehavior);

  const TextIteratorBehavior behavior_;
  const EphemeralRangeInFlatTree range_;
  const String text16_;

  DISALLOW_COPY_AND_ASSIGN(TextOffsetMapping);
};

PositionInFlatTree ComputeEndPosition(const LayoutBlock&);
PositionInFlatTree ComputeStartPosition(const LayoutBlock&);

}  // namespace blink

#endif  // TextOffsetMapping_h
