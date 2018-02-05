// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/blink/snap_fling_curve.h"

#include <cmath>

namespace ui {
namespace {

// Based on statistics from hundreds of flings. The distance left to be flinged
// is about 10 times of the current delta.
const double kDistanceEstimatorScalar = 16;
const double kFrameIntervalMs = 16;

inline double EstimateFramesFromDistance(double distance) {
  return (std::log(kDistanceEstimatorScalar) - std::log(distance)) /
             std::log(1 - 1 / kDistanceEstimatorScalar) +
         kDistanceEstimatorScalar;
}

inline double GetFrameFromTime(base::TimeTicks time_stamp,
                               base::TimeTicks start) {
  return (time_stamp - start).InMillisecondsF() / kFrameIntervalMs + 1;
}

inline double GetDistanceFromDisplacement(gfx::Vector2dF displacement) {
  return std::hypot(displacement.x(), displacement.y());
}

}  // namespace

gfx::Vector2dF SnapFlingCurve::EstimateDisplacement(
    const gfx::Vector2dF& first_delta) {
  gfx::Vector2dF destination = first_delta;
  destination.Scale(kDistanceEstimatorScalar);
  return destination;
}

SnapFlingCurve::SnapFlingCurve(const gfx::Vector2dF& displacement,
                               base::TimeTicks time_stamp)
    : total_displacement_(displacement),
      natural_displacement_(gfx::Vector2dF()),
      actual_displacement_(gfx::Vector2dF()),
      start_time_(time_stamp),
      total_distance_(GetDistanceFromDisplacement(displacement)),
      curve_distance_(0),
      total_frames_(EstimateFramesFromDistance(total_distance_)),
      current_frame_(0),
      is_finished_(total_distance_ == 0) {
  if (is_finished_)
    return;
  ratio_x_ = displacement.x() / total_distance_;
  ratio_y_ = displacement.y() / total_distance_;
}

SnapFlingCurve::~SnapFlingCurve() = default;

double SnapFlingCurve::GetCurrentCurveDistance() {
  if (current_frame_ >= total_frames_)
    return total_distance_;

  if (total_distance_ - curve_distance_ < 1)
    return total_distance_;
  if (total_distance_ - curve_distance_ <= kDistanceEstimatorScalar)
    return curve_distance_ + 1;

  return total_distance_ *
         (1 - std::pow((1 - 1.0 / kDistanceEstimatorScalar), current_frame_));
}

double SnapFlingCurve::WeightDistance(double curve_distance,
                                      double natural_distance,
                                      double current_frame) {
  if (current_frame >= total_frames_)
    return total_distance_;
  double curve_ratio = current_frame / total_frames_;
  double natural_ratio = 1 - curve_ratio;
  return curve_ratio * curve_distance + natural_ratio * natural_distance;
}

gfx::Vector2dF SnapFlingCurve::SnapFlingDelta(
    const gfx::Vector2dF& natural_delta,
    base::TimeTicks time_stamp) {
  if (is_finished_)
    return gfx::Vector2dF();

  current_frame_ = GetFrameFromTime(time_stamp, start_time_);

  natural_displacement_ += natural_delta;
  double natural_distance = GetDistanceFromDisplacement(natural_displacement_);
  curve_distance_ = GetCurrentCurveDistance();
  double weighted_distance =
      WeightDistance(curve_distance_, natural_distance, current_frame_);
  gfx::Vector2dF weighted_displacement(weighted_distance * ratio_x_,
                                       weighted_distance * ratio_y_);
  if (GetDistanceFromDisplacement(weighted_displacement) >= total_distance_)
    weighted_displacement = total_displacement_;

  gfx::Vector2dF snapped_delta = weighted_displacement - actual_displacement_;
  return snapped_delta;
}

void SnapFlingCurve::UpdateScrolledDelta(const gfx::Vector2dF& scrolled_delta) {
  actual_displacement_ += scrolled_delta;
  if (current_frame_ >= total_frames_ ||
      (current_frame_ >= total_frames_ - kDistanceEstimatorScalar &&
       actual_displacement_ == total_displacement_)) {
    is_finished_ = true;
  }
}

}  // namespace ui
