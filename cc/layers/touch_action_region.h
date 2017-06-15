// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_LAYERS_TOUCH_ACTION_REGION_H_
#define CC_LAYERS_TOUCH_ACTION_REGION_H_

#include "base/values.h"
#include "cc/base/region.h"
#include "cc/cc_export.h"
#include "cc/input/touch_action.h"
#include "ui/gfx/geometry/rect.h"

namespace cc {

class CC_EXPORT TouchActionRegion {
 public:
  TouchActionRegion();
  TouchActionRegion(const TouchActionRegion& touch_action_region);
  TouchActionRegion(TouchActionRegion&& touch_action_region);
  ~TouchActionRegion();

  void Union(TouchAction, const gfx::Rect&);
  const Region& region() const { return region_; }
  const Region& GetRegionForTouchAction(TouchAction);

  TouchActionRegion& operator=(const TouchActionRegion& other);
  TouchActionRegion& operator=(TouchActionRegion&& other);
  bool operator==(const TouchActionRegion& other) const;

 private:
  base::flat_map<TouchAction, Region> map_;
  Region region_;
};

}  // namespace cc

#endif  // CC_LAYERS_TOUCH_ACTION_REGION_H_
