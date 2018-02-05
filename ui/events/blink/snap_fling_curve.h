// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_BLINK_SNAP_FLING_CURVE_H_
#define UI_EVENTS_BLINK_SNAP_FLING_CURVE_H_

#include "base/time/time.h"
#include "ui/gfx/geometry/vector2d_f.h"

namespace ui {

class SnapFlingCurve {
 public:
  SnapFlingCurve(const gfx::Vector2dF& distance, base::TimeTicks time_stamp);

  ~SnapFlingCurve();

  static gfx::Vector2dF EstimateDisplacement(const gfx::Vector2dF& first_delta);

  gfx::Vector2dF SnapFlingDelta(const gfx::Vector2dF& displacement,
                                base::TimeTicks time);
  void UpdateScrolledDelta(const gfx::Vector2dF& scrolled_delta);
  bool IsFinished() const { return is_finished_; }

 private:
  double GetCurrentCurveDistance();
  double WeightDistance(double curve_distance,
                        double natural_distance,
                        double current_frame);

  gfx::Vector2dF total_displacement_;
  gfx::Vector2dF natural_displacement_;
  gfx::Vector2dF actual_displacement_;
  base::TimeTicks start_time_;
  double total_distance_;
  double curve_distance_;
  double total_frames_;
  double current_frame_;
  bool is_finished_;
  double ratio_x_;
  double ratio_y_;
};

}  // namespace ui

#endif  // UI_EVENTS_BLINK_SNAP_FLING_CURVE_H_
