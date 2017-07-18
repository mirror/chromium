// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LayoutBoxPainterInterface_h
#define LayoutBoxPainterInterface_h

#include "core/paint/BoxModelObjectPainter.h"
#include "core/paint/LayoutBoxModelObjectPainterInterface.h"

namespace blink {

class LayoutBoxModelObject;

// Legacy layout implementation of the BoxPainterInterface.
class LayoutBoxPainterInterface
    : public virtual LayoutBoxModelObjectPainterInterface,
      public BoxPainterInterface {
 public:
  LayoutBoxPainterInterface(const LayoutBox&);

  bool IsBox() const override { return true; }
  LayoutSize LocationOffset() const override;
  IntSize ScrollOffset() const override;
  LayoutSize ScrollSize() const override;
  LayoutRect OverflowClipRect(const LayoutPoint&) const override;

  const LayoutBox& GetLayoutBox() const { return layout_box_; }
  const BoxPainterInterface* ToBoxPainterInterface() const override {
    return this;
  }

 private:
  const LayoutBox& layout_box_;
};

}  // namespace blink

#endif  // LayoutBoxPainterInterface_h
