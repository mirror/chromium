// Copyright 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layers/touch_action_region.h"

namespace cc {

static const Region empty_region_;

TouchActionRegion::TouchActionRegion() = default;
TouchActionRegion::TouchActionRegion(
    const TouchActionRegion& touch_action_region) = default;
TouchActionRegion::TouchActionRegion(TouchActionRegion&& touch_action_region) =
    default;
TouchActionRegion::~TouchActionRegion() = default;

void TouchActionRegion::Union(TouchAction touch_action, const gfx::Rect& rect) {
  region_.Union(rect);
  map_[touch_action].Union(rect);
}

const Region& TouchActionRegion::GetRegionForTouchAction(
    TouchAction touch_action) {
  auto it = map_.find(touch_action);
  if (it == map_.end())
    return empty_region_;
  return it->second;
}

TouchActionRegion& TouchActionRegion::operator=(
    const TouchActionRegion& other) = default;
TouchActionRegion& TouchActionRegion::operator=(TouchActionRegion&& other) =
    default;

bool TouchActionRegion::operator==(const TouchActionRegion& other) const {
  return map_ == other.map_;
}

}  // namespace cc
