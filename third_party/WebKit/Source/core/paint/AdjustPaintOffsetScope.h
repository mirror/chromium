// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef AdjustPaintOffsetScope_h
#define AdjustPaintOffsetScope_h

#include "core/layout/LayoutBox.h"
#include "core/paint/PaintInfo.h"
#include "core/paint/ng/ng_paint_fragment.h"
#include "platform/graphics/paint/ScopedPaintChunkProperties.h"

namespace blink {

class ScopedPaintChunkProperties;

class AdjustPaintOffsetScope {
  STACK_ALLOCATED();

 public:
  AdjustPaintOffsetScope(const LayoutBox& box,
                         const PaintInfo& paint_info,
                         const LayoutPoint& paint_offset)
      : old_paint_info_(paint_info) {
    if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled() ||
        !ShouldAdjustForPaintOffsetTranslation(box)) {
      adjusted_paint_offset_ = paint_offset + box.Location();
      return;
    }

    AdjustForPaintOffsetTranslation(box);
  }

  AdjustPaintOffsetScope(const NGPaintFragment& paint_fragment,
                         const PaintInfo& paint_info,
                         const LayoutPoint& paint_offset)
      : old_paint_info_(paint_info) {
    // A box fragment can be eitehr a block box or an inline box.
    DCHECK_EQ(NGPhysicalFragment::kFragmentBox,
              paint_fragment.PhysicalFragment().Type());
    const LayoutObject* layout_object = paint_fragment.GetLayoutObject();
    DCHECK(layout_object && layout_object->IsBoxModelObject());
    if (!layout_object->IsBox()) {
      // TODO(kojii): What is the expected behavior of inline boxes when
      // SlimmingPaintV175Enabled?
      DCHECK(!RuntimeEnabledFeatures::SlimmingPaintV175Enabled() ||
             !ShouldAdjustForPaintOffsetTranslation(
                 ToLayoutBoxModelObject(*layout_object)));
      adjusted_paint_offset_ =
          paint_offset + paint_fragment.Offset().ToLayoutPoint();
      return;
    }

    const LayoutBox& box = ToLayoutBox(*layout_object);
    if (!RuntimeEnabledFeatures::SlimmingPaintV175Enabled() ||
        !ShouldAdjustForPaintOffsetTranslation(box)) {
      adjusted_paint_offset_ = paint_offset;
      if (layout_object->IsTableCell()) {
        // TODO(kojii): LayoutNGTableCell does not compute fragment.Offset()
        // correctly, and that we need to use LayoutBox::Location() instead. We
        // should use fragment offset once it's fixed.
        adjusted_paint_offset_ += box.Location();
      } else {
        adjusted_paint_offset_ += paint_fragment.Offset().ToLayoutPoint();
      }
      return;
    }

    AdjustForPaintOffsetTranslation(box);
  }

  const PaintInfo& GetPaintInfo() const {
    return new_paint_info_ ? *new_paint_info_ : old_paint_info_;
  }

  PaintInfo& MutablePaintInfo() {
    if (!new_paint_info_)
      new_paint_info_.emplace(old_paint_info_);
    return *new_paint_info_;
  }

  LayoutPoint AdjustedPaintOffset() const { return adjusted_paint_offset_; }

 private:
  static bool ShouldAdjustForPaintOffsetTranslation(
      const LayoutBoxModelObject&);
  void AdjustForPaintOffsetTranslation(const LayoutBox&);

  const PaintInfo& old_paint_info_;
  LayoutPoint adjusted_paint_offset_;
  Optional<PaintInfo> new_paint_info_;
  Optional<ScopedPaintChunkProperties> contents_properties_;
};

}  // namespace blink

#endif  // AdjustPaintOffsetScope_h
