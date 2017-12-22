// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_OZONE_EVDEV_TOUCH_FILTER_TOUCH_FILTER_H_
#define UI_EVENTS_OZONE_EVDEV_TOUCH_FILTER_TOUCH_FILTER_H_

#include <bitset>
#include <vector>

#include "base/time/time.h"
#include "ui/events/ozone/evdev/touch_evdev_types.h"

namespace ui {

class TouchFilter {
 public:
  virtual ~TouchFilter() {}
  virtual void Filter(const std::vector<InProgressTouchEvdev>& touches,
                      base::TimeTicks time,
                      std::bitset<kNumTouchEvdevSlots>* slots_to_suppress) = 0;

 protected:
  // Returns the squared distance between (|x1|, |y1|) and (|x2|, |y2|)
  static int Distance2(int x1, int y1, int x2, int y2) {
    int offset_x = x2 - x1;
    int offset_y = y2 - y1;
    return offset_x * offset_x + offset_y * offset_y;
  }
};

}  // namespace ui

#endif  // UI_EVENTS_OZONE_EVDEV_TOUCH_FILTER_TOUCH_FILTER_H_
