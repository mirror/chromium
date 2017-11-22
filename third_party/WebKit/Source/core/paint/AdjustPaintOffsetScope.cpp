// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/paint/AdjustPaintOffsetScope.h"

namespace blink {

bool AdjustPaintOffsetScope::AdjustForPaintOffsetTranslation(
    const LayoutBox& box) {
  DCHECK(RuntimeEnabledFeatures::SlimmingPaintV175Enabled());

  if (box.HasSelfPaintingLayer())
    return false;

  const auto* paint_properties = old_paint_info_.Fragment().PaintProperties();
  if (!paint_properties || !paint_properties->PaintOffsetTranslation())
    return false;

  const auto* local_border_box_properties =
      old_paint_info_.Fragment().LocalBorderBoxProperties();
  PaintChunkProperties chunk_properties(
      old_paint_info_.context.GetPaintController()
          .CurrentPaintChunkProperties());
  chunk_properties.property_tree_state = *local_border_box_properties;
  contents_properties_.emplace(old_paint_info_.context.GetPaintController(),
                               box, chunk_properties);

  new_paint_info_.emplace(old_paint_info_);
  new_paint_info_->UpdateCullRect(
      paint_properties->PaintOffsetTranslation()->Matrix().ToAffineTransform());

  adjusted_paint_offset_ = old_paint_info_.Fragment().PaintOffset();
  return true;
}

}  // namespace blink
