// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGUnits_h
#define NGUnits_h

#include "core/CoreExport.h"
#include "core/layout/ng/ng_direction.h"
#include "core/layout/ng/ng_writing_mode.h"
#include "platform/LayoutUnit.h"

namespace blink {

class LayoutUnit;
struct NGPhysicalOffset;
struct NGPhysicalSize;

struct NGLogicalSize {
  NGLogicalSize() {}
  NGLogicalSize(LayoutUnit inline_size, LayoutUnit block_size)
      : inline_size(inline_size), block_size(block_size) {}

  LayoutUnit inline_size;
  LayoutUnit block_size;

  NGPhysicalSize ConvertToPhysical(NGWritingMode mode) const;
};

// NGLogicalOffset is the position of a rect (typically a fragment) relative to
// its parent rect in the logical coordinate system.
struct NGLogicalOffset {
  NGLogicalOffset() {}
  NGLogicalOffset(LayoutUnit inline_offset, LayoutUnit block_offset)
      : inline_offset(inline_offset), block_offset(block_offset) {}

  LayoutUnit inline_offset;
  LayoutUnit block_offset;

  // Converts a logical offset to a physical offset. See:
  // https://drafts.csswg.org/css-writing-modes-3/#logical-to-physical
  // @param container_size the size of the rect (typically a fragment).
  // @param inner_size the size of the inner rect (typically a child fragment).
  CORE_EXPORT NGPhysicalOffset
  ConvertToPhysical(NGWritingMode mode,
                    NGDirection direction,
                    NGPhysicalSize container_size,
                    NGPhysicalSize inner_size) const;
};

// NGPhysicalOffset is the position of a rect (typically a fragment) relative to
// its parent rect in the physical coordinate system.
struct NGPhysicalOffset {
  NGPhysicalOffset() {}
  NGPhysicalOffset(LayoutUnit left, LayoutUnit top) : left(left), top(top) {}

  LayoutUnit left;
  LayoutUnit top;
};

struct NGPhysicalSize {
  NGPhysicalSize() {}
  NGPhysicalSize(LayoutUnit width, LayoutUnit height)
      : width(width), height(height) {}

  LayoutUnit width;
  LayoutUnit height;

  NGLogicalSize ConvertToLogical(NGWritingMode mode) const;
};

// NGPhysicalLocation is the position of a rect (typically a fragment) relative
// to the root document.
struct NGPhysicalLocation {
  LayoutUnit left;
  LayoutUnit top;
};

struct NGPhysicalRect {
  NGPhysicalSize size;
  NGPhysicalLocation location;
};

struct NGPixelSnappedPhysicalRect {
  int top;
  int left;
  int width;
  int height;
};

// This struct is used for storing margins, borders or padding of a box on all
// four edges.
struct NGBoxStrut {
  LayoutUnit inline_start;
  LayoutUnit inline_end;
  LayoutUnit block_start;
  LayoutUnit block_end;

  LayoutUnit InlineSum() const { return inline_start + inline_end; }
  LayoutUnit BlockSum() const { return block_start + block_end; }

  // The following two operators exist primarily to have an easy way to access
  // the sum of border and padding.
  NGBoxStrut& operator+=(const NGBoxStrut& other) {
    inline_start += other.inline_start;
    inline_end += other.inline_end;
    block_start += other.block_start;
    block_end += other.block_end;
    return *this;
  }

  NGBoxStrut operator+(const NGBoxStrut& other) {
    NGBoxStrut result(*this);
    result += other;
    return result;
  }
};

}  // namespace blink

#endif  // NGUnits_h
