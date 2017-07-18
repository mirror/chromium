// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/LayoutBoxPainterInterface.h"

#include "core/layout/LayoutBox.h"

namespace blink {

LayoutBoxPainterInterface::LayoutBoxPainterInterface(const LayoutBox& box)
    : LayoutBoxModelObjectPainterInterface(box), layout_box_(box) {}

LayoutSize LayoutBoxPainterInterface::LocationOffset() const {
  return layout_box_.LocationOffset();
}

IntSize LayoutBoxPainterInterface::ScrollOffset() const {
  return layout_box_.ScrolledContentOffset();
}

LayoutSize LayoutBoxPainterInterface::ScrollSize() const {
  return LayoutSize(layout_box_.ScrollWidth(), layout_box_.ScrollHeight());
}

LayoutRect LayoutBoxPainterInterface::OverflowClipRect(
    const LayoutPoint& location) const {
  return layout_box_.OverflowClipRect(location);
}

}  // namespace blink
