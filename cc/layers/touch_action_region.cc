// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/touch_action_region.h"

#include "ui/gfx/geometry/rect.h"

namespace cc {

TouchActionRegion::TouchActionRegion() : region_(new Region()) {}
TouchActionRegion::TouchActionRegion(
    const TouchActionRegion& touch_action_region)
    : map_(touch_action_region.map_),
      region_(new Region(touch_action_region.region())) {}
TouchActionRegion::TouchActionRegion(TouchActionRegion&& touch_action_region)
    : map_(std::move(touch_action_region.map_)),
      region_(new Region(std::move(touch_action_region.region()))) {}
TouchActionRegion::~TouchActionRegion() = default;

void TouchActionRegion::Union(TouchAction touch_action, const gfx::Rect& rect) {
  region_->Union(rect);
  map_[touch_action].Union(rect);
}

const Region& TouchActionRegion::GetRegionForTouchAction(
    TouchAction touch_action) {
  static const Region* r = new Region;
  auto it = map_.find(touch_action);
  if (it == map_.end())
    return *r;
  return it->second;
}

TouchActionRegion& TouchActionRegion::operator=(
    const TouchActionRegion& other) {
  *region_ = *other.region_;
  map_ = other.map_;
  return *this;
}

TouchActionRegion& TouchActionRegion::operator=(TouchActionRegion&& other) =
    default;

bool TouchActionRegion::operator==(const TouchActionRegion& other) const {
  return map_ == other.map_;
}

}  // namespace cc
