// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/layout/ng/geometry/ng_physical_rect.h"

#include "platform/geometry/LayoutRect.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

NGPixelSnappedPhysicalRect NGPhysicalRect::SnapToDevicePixels() const {
  NGPixelSnappedPhysicalRect snapped_rect;
  snapped_rect.left = RoundToInt(location.left);
  snapped_rect.top = RoundToInt(location.top);
  snapped_rect.width = SnapSizeToPixel(size.width, location.left);
  snapped_rect.height = SnapSizeToPixel(size.height, location.top);

  return snapped_rect;
}

bool NGPhysicalRect::operator==(const NGPhysicalRect& other) const {
  return other.location == location && other.size == size;
}

void NGPhysicalRect::Unite(const NGPhysicalRect& other) {
  if (other.IsEmpty())
    return;
  if (IsEmpty()) {
    *this = other;
    return;
  }

  LayoutUnit left = std::min(location.left, other.location.left);
  LayoutUnit top = std::min(location.top, other.location.top);
  LayoutUnit right = std::max(Right(), other.Right());
  LayoutUnit bottom = std::max(Bottom(), other.Bottom());
  location = {left, top};
  size = {right - left, bottom - top};
}

NGPhysicalRect::NGPhysicalRect(const LayoutRect& source)
    : NGPhysicalRect({source.X(), source.Y()},
                     {source.Width(), source.Height()}) {}

LayoutRect NGPhysicalRect::ToLayoutRect() const {
  return {location.left, location.top, size.width, size.height};
}

String NGPhysicalRect::ToString() const {
  return String::Format("%s,%s %sx%s", location.left.ToString().Ascii().data(),
                        location.top.ToString().Ascii().data(),
                        size.width.ToString().Ascii().data(),
                        size.height.ToString().Ascii().data());
}

std::ostream& operator<<(std::ostream& os, const NGPhysicalRect& value) {
  return os << value.ToString();
}

}  // namespace blink
