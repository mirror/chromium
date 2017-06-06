// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/paint_context.h"

#include "ui/gfx/canvas.h"

namespace ui {

PaintContext::PaintContext(cc::DisplayItemList* list,
                           float device_scale_factor,
                           const gfx::Rect& invalidation,
                           const gfx::Size& size)
    : list_(list),
      owned_recorder_(new cc::PaintRecorder),
      recorder_(owned_recorder_.get()),
      device_scale_factor_(device_scale_factor),
      invalidation_(gfx::ScaleToEnclosingRect(
          invalidation,
          IsPixelCanvas() ? device_scale_factor : 1.f)),
      bounds_(size),
      pixel_bounds_(gfx::ScaleToEnclosingRect(
          bounds_,
          IsPixelCanvas() ? device_scale_factor : 1.f)) {
#if DCHECK_IS_ON()
  root_visited_ = nullptr;
  inside_paint_recorder_ = false;
#endif
}

PaintContext::PaintContext(const PaintContext& other,
                           const gfx::Rect& bounds,
                           int scale_type)
    : list_(other.list_),
      owned_recorder_(nullptr),
      recorder_(other.recorder_),
      device_scale_factor_(other.device_scale_factor_),
      invalidation_(other.invalidation_),
      bounds_(bounds),
      pixel_bounds_(other.GetSnappedPixelBounds(bounds_, scale_type)),
      offset_(other.offset_ + pixel_bounds_.OffsetFromOrigin()),
      scale_type_(scale_type) {
#if DCHECK_IS_ON()
  root_visited_ = other.root_visited_;
  inside_paint_recorder_ = other.inside_paint_recorder_;
#endif
}

PaintContext::PaintContext(const PaintContext& other,
                           CloneWithoutInvalidation c)
    : list_(other.list_),
      owned_recorder_(nullptr),
      recorder_(other.recorder_),
      device_scale_factor_(other.device_scale_factor_),
      invalidation_(),
      bounds_(other.bounds_),
      pixel_bounds_(other.pixel_bounds_),
      offset_(other.offset_) {
#if DCHECK_IS_ON()
  root_visited_ = other.root_visited_;
  inside_paint_recorder_ = other.inside_paint_recorder_;
#endif
}

PaintContext::~PaintContext() {
}

gfx::Rect PaintContext::ToLayerSpaceBounds(
    const gfx::Size& size_in_context) const {
  return gfx::Rect(size_in_context) + offset_;
}

float PaintContext::effective_scale_factor_x() const {
  if (pixel_bounds_.width() == 0)
    return IsPixelCanvas() ? device_scale_factor_ : 1.f;

  if (scale_type_ == SCALE_TO_SCALE_FACTOR)
    return device_scale_factor_;

  return (float)pixel_bounds_.width() / (float)bounds_.width();
}

float PaintContext::effective_scale_factor_y() const {
  if (pixel_bounds_.height() == 0)
    return IsPixelCanvas() ? device_scale_factor_ : 1.f;

  if (scale_type_ == SCALE_TO_SCALE_FACTOR)
    return device_scale_factor_;

  return (float)pixel_bounds_.height() / (float)bounds_.height();
}

gfx::Size PaintContext::ScaleToEffectivePixelSize(const gfx::Size& size) const {
  // effective_scale_factor_x/y() would return 1.f value if pixel canvas is
  // disabled. This early return is only an optimization.
  if (!IsPixelCanvas())
    return size;
  return gfx::ScaleToCeiledSize(size, effective_scale_factor_x(),
                                effective_scale_factor_y());
}

gfx::Rect PaintContext::ScaleToEffectivePixelBounds(
    const gfx::Rect& bounds) const {
  // effective_scale_factor_x/y() would return 1.f value if pixel canvas is
  // disabled. This early return is only an optimization.
  if (!IsPixelCanvas())
    return bounds;
  return gfx::ScaleToEnclosingRect(bounds, effective_scale_factor_x(),
                                   effective_scale_factor_y());
}

gfx::Rect PaintContext::GetSnappedPixelBounds(const gfx::Rect& child_bounds,
                                              int scale_type) const {
  if (!IsPixelCanvas())
    return child_bounds;

  if (scale_type == SCALE_TO_SCALE_FACTOR) {
    return gfx::ScaleToEnclosingRect(child_bounds, device_scale_factor_);
  }

  const gfx::Vector2d& child_origin = child_bounds.OffsetFromOrigin();

  int right = child_origin.x() + child_bounds.width();
  int bottom = child_origin.y() + child_bounds.height();

  int new_x = std::round(child_origin.x() * device_scale_factor_);
  int new_y = std::round(child_origin.y() * device_scale_factor_);

  int new_right;
  int new_bottom;

  if (right == bounds_.width()) {
    new_right = pixel_bounds_.width();
  } else {
    new_right = std::round(right * device_scale_factor_);
  }

  if (bottom == bounds_.height()) {
    new_bottom = pixel_bounds_.height();
  } else {
    new_bottom = std::round(bottom * device_scale_factor_);
  }
  return gfx::Rect(new_x, new_y, new_right - new_x, new_bottom - new_y);
}

}  // namespace ui
