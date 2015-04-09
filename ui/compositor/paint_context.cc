// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/paint_context.h"

#include "ui/gfx/canvas.h"

namespace ui {

PaintContext::PaintContext(gfx::Canvas* canvas, const gfx::Rect& invalidation)
    : canvas_(canvas),
      list_(nullptr),
      device_scale_factor_(canvas->image_scale()),
      bounds_(invalidation),
      invalidation_(invalidation) {
#if DCHECK_IS_ON()
  root_visited_ = nullptr;
#endif
}

PaintContext::PaintContext(cc::DisplayItemList* list,
                           float device_scale_factor,
                           const gfx::Rect& bounds,
                           const gfx::Rect& invalidation)
    : canvas_(nullptr),
      list_(list),
      device_scale_factor_(device_scale_factor),
      bounds_(bounds),
      invalidation_(invalidation) {
#if DCHECK_IS_ON()
  root_visited_ = nullptr;
#endif
}

PaintContext::PaintContext(gfx::Canvas* canvas)
    : PaintContext(canvas, gfx::Rect()) {
}

PaintContext::PaintContext(const PaintContext& other)
    : canvas_(other.canvas_),
      list_(other.list_),
      device_scale_factor_(other.device_scale_factor_),
      bounds_(other.bounds_),
      invalidation_(other.invalidation_),
      offset_(other.offset_) {
#if DCHECK_IS_ON()
  root_visited_ = other.root_visited_;
#endif
}

PaintContext::PaintContext(const PaintContext& other,
                           const gfx::Vector2d& offset)
    : PaintContext(other) {
  offset_ += offset;
}

PaintContext::~PaintContext() {
}

}  // namespace ui
