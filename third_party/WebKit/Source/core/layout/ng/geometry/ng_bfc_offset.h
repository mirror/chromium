// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGBfcOffset_h
#define NGBfcOffset_h

#include "core/CoreExport.h"
#include "platform/LayoutUnit.h"
#include "platform/text/TextDirection.h"

namespace blink {

struct NGLogicalOffset;

// TODO FIXME comment.
// NGBfcOffset is the position of a rect (typically a fragment) relative to
// its parent rect in the logical coordinate system.
struct CORE_EXPORT NGBfcOffset {
  NGBfcOffset() {}
  NGBfcOffset(LayoutUnit line_offset, LayoutUnit block_offset)
      : line_offset(line_offset), block_offset(block_offset) {}

  LayoutUnit line_offset;
  LayoutUnit block_offset;

  // TODO FIXME comment.
  // Converts a logical offset to a physical offset. See:
  // https://drafts.csswg.org/css-writing-modes-3/#logical-to-physical
  // PhysicalOffset will be the physical top left point of the rectangle
  // described by offset + inner_size. Setting inner_size to 0,0 will return
  // the same point.
  // @param outer_size the size of the rect (typically a fragment).
  // @param inner_size the size of the inner rect (typically a child fragment).
  NGLogicalOffset ConvertToLogical(TextDirection,
                                   LayoutUnit outer_size,
                                   LayoutUnit inner_size) const;

  bool operator==(const NGBfcOffset& other) const;
  bool operator!=(const NGBfcOffset& other) const;

  String ToString() const;
};

CORE_EXPORT std::ostream& operator<<(std::ostream&, const NGBfcOffset&);

}  // namespace blink

#endif  // NGBfcOffset_h
