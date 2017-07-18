// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_OZONE_EVDEV_TOUCH_FILTER_INCONSISTENT_SPEED_FILTER_H_
#define UI_EVENTS_OZONE_EVDEV_TOUCH_FILTER_INCONSISTENT_SPEED_FILTER_H_

#include <deque>

#include "base/macros.h"
#include "base/time/time.h"

namespace ui {

struct InProgressTouchEvdev;

// This filter will adjust timestamps to maintain consistent velocity of touch
// movement.
//
// The algorithm achieves this without added latency by adjusting the timestamps
// to result in a consistent velocity.
//
// It is used on some Wacom devices, which have irregular report rates causing
// irregularities in the reported velocity.
//
// Note: While this might add 'latency' in measurements that use the timestamp
// as a reference, it will not actually add latency for the user.
class InconsistentSpeedFilter {
 public:
  // The kernel size should be chosen to match the scanning pattern of the
  // sensor.
  InconsistentSpeedFilter(size_t kernel_size);
  virtual ~InconsistentSpeedFilter() {}

  base::TimeTicks FilterTimestamp(base::TimeTicks timestamp,
                                  const InProgressTouchEvdev& touch);

 private:
  class MovingAverageWindow {
   public:
    explicit MovingAverageWindow(size_t kernel_size);
    ~MovingAverageWindow();
    void Add(float value);
    void Clear();
    float Average();

   private:
    size_t kernel_size_;
    std::deque<float> buffer_;
  };

  void Reset();
  float CalculateCorrectedDeltaT(float dt_sensor, float dxy, float v_sensor);

  base::TimeTicks previous_t_sensor_;
  base::TimeTicks previous_t_corrected_;
  float previous_x_;
  float previous_y_;

  MovingAverageWindow velocity_window_;
  MovingAverageWindow drift_window_;

  DISALLOW_COPY_AND_ASSIGN(InconsistentSpeedFilter);
};

}  // namespace ui

#endif  // UI_EVENTS_OZONE_EVDEV_TOUCH_FILTER_INCONSISTENT_SPEED_FILTER_H_
