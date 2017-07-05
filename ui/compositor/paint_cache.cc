// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/paint_cache.h"

#include "cc/paint/display_item_list.h"
#include "cc/paint/paint_op_buffer.h"
#include "ui/compositor/paint_context.h"

namespace ui {

PaintCache::PaintCache() = default;
PaintCache::~PaintCache() = default;

bool PaintCache::UseCache(const PaintContext& context,
                          const gfx::Size& size_in_context) {
  if (!paint_op_buffer_)
    return false;
  DCHECK(context.list_);
  context.list_->StartPaint();
  context.list_->push<cc::DrawRecordOp>(paint_op_buffer_);
  gfx::Rect bounds_in_layer = context.ToLayerSpaceBounds(size_in_context);
  context.list_->EndPaintOfUnpaired(bounds_in_layer);
  return true;
}

cc::DisplayItemList* PaintCache::ResetCache() {
  paint_op_buffer_ = nullptr;
  display_item_list_ = base::MakeRefCounted<cc::DisplayItemList>(
      cc::DisplayItemList::kToBeReleasedAsPaintOpBuffer);
  display_item_list_->StartPaint();
  return display_item_list_.get();
}

void PaintCache::FinalizeCache() {
  // The bounds of the unpainred call don't matter, since we immediately
  // release the list as a paint op buffer.
  display_item_list_->EndPaintOfUnpaired(gfx::Rect());
  display_item_list_->Finalize();
  paint_op_buffer_ = display_item_list_->ReleaseAsRecord();
  display_item_list_ = nullptr;
}

}  // namespace ui
