// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/geometry/ng_logical_rect.h"

#include "platform/wtf/text/WTFString.h"

namespace blink {

namespace {

inline NGLogicalOffset Min(NGLogicalOffset a, NGLogicalOffset b) {
  return {std::min(a.inline_offset, b.inline_offset),
          std::min(a.block_offset, b.block_offset)};
}

inline NGLogicalOffset Max(NGLogicalOffset a, NGLogicalOffset b) {
  return {std::max(a.inline_offset, b.inline_offset),
          std::max(a.block_offset, b.block_offset)};
}

}  // namespace

bool NGLogicalRect::operator==(const NGLogicalRect& other) const {
  return other.offset == offset && other.size == size;
}

void NGLogicalRect::Unite(const NGLogicalRect& other) {
  if (other.IsEmpty())
    return;
  if (IsEmpty()) {
    *this = other;
    return;
  }

  NGLogicalOffset new_end_offset(Max(EndOffset(), other.EndOffset()));
  offset = Min(offset, other.offset);
  size = new_end_offset - offset;
}

String NGLogicalRect::ToString() const {
  return String::Format("%s,%s %sx%s",
                        offset.inline_offset.ToString().Ascii().data(),
                        offset.block_offset.ToString().Ascii().data(),
                        size.inline_size.ToString().Ascii().data(),
                        size.block_size.ToString().Ascii().data());
}

std::ostream& operator<<(std::ostream& os, const NGLogicalRect& value) {
  return os << value.ToString();
}

}  // namespace blink
