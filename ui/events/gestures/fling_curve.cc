// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/gestures/fling_curve.h"

#include <algorithm>
#include <cmath>

#include "base/logging.h"

namespace {

const float kDefaultAlpha = -5.70762e+03f;
const float kDefaultBeta = 1.72e+02f;
const float kDefaultGamma = 3.7e+00f;

inline double GetPositionAtTime(double t) {
  return kDefaultAlpha * exp(-kDefaultGamma * t) - kDefaultBeta * t -
         kDefaultAlpha;
}

inline double GetVelocityAtTime(double t) {
  return -kDefaultAlpha * kDefaultGamma * exp(-kDefaultGamma * t) -
         kDefaultBeta;
}

inline double GetTimeAtVelocity(double v) {
  return -log((v + kDefaultBeta) / (-kDefaultAlpha * kDefaultGamma)) /
         kDefaultGamma;
}

double GetTimeAtPosition(double p) {
  // TODO(sunyunjia): Set a range for the time for faster interpolation.
  double tl = 0;
  double tr = GetTimeAtVelocity(0);
  p = GetPositionAtTime(tr) - p;
  double tm = (tl + tr) / 2;
  double pm = GetPositionAtTime(tm);
  while (fabs(pm - p) > 0.001) {
    if (pm < p)
      tl = tm;
    else
      tr = tm;
    tm = (tl + tr) / 2;
    pm = GetPositionAtTime(tm);
  }
  return tm;
}

}  // namespace

namespace ui {

FlingCurve::FlingCurve(base::TimeTicks start_timestamp,
                       const gfx::Vector2dF& displacement_ratio,
                       float time_offset,
                       float position_offset)
    : curve_duration_(GetTimeAtVelocity(0)),
      start_timestamp_(start_timestamp),
      displacement_ratio_(displacement_ratio),
      previous_timestamp_(start_timestamp),
      time_offset_(time_offset),
      position_offset_(position_offset) {}

// static
std::unique_ptr<FlingCurve> FlingCurve::CreateFromVelocity(
    const gfx::Vector2dF& velocity,
    base::TimeTicks start_timestamp) {
  DCHECK(!velocity.IsZero());
  float max_start_velocity = std::max(fabs(velocity.x()), fabs(velocity.y()));
  if (max_start_velocity > GetVelocityAtTime(0))
    max_start_velocity = GetVelocityAtTime(0);
  CHECK_GT(max_start_velocity, 0);

  gfx::Vector2dF displacement_ratio = gfx::Vector2dF(
      velocity.x() / max_start_velocity, velocity.y() / max_start_velocity);
  float time_offset = GetTimeAtVelocity(max_start_velocity);
  float position_offset = GetPositionAtTime(time_offset);
  return std::unique_ptr<FlingCurve>(new FlingCurve(
      start_timestamp, displacement_ratio, time_offset, position_offset));
}

// static
std::unique_ptr<FlingCurve> FlingCurve::CreateFromDistance(
    const gfx::Vector2dF& distance,
    base::TimeTicks start_timestamp) {
  DCHECK(!distance.IsZero());
  float max_offset = std::max(fabs(distance.x()), fabs(distance.y()));
  if (max_offset > GetPositionAtTime(0))
    return nullptr;

  gfx::Vector2dF displacement_ratio =
      gfx::Vector2dF(distance.x() / max_offset, distance.y() / max_offset);

  float time_offset = GetTimeAtPosition(max_offset);
  float position_offset = GetPositionAtTime(time_offset);
  return std::unique_ptr<FlingCurve>(new FlingCurve(
      start_timestamp, displacement_ratio, time_offset, position_offset));
}

FlingCurve::~FlingCurve() {
}

bool FlingCurve::ComputeScrollOffset(base::TimeTicks time,
                                     gfx::Vector2dF* offset,
                                     gfx::Vector2dF* velocity) {
  DCHECK(offset);
  DCHECK(velocity);
  base::TimeDelta elapsed_time = time - start_timestamp_;
  if (elapsed_time < base::TimeDelta()) {
    *offset = gfx::Vector2dF();
    *velocity = gfx::Vector2dF();
    return true;
  }

  bool still_active = true;
  float scalar_offset;
  float scalar_velocity;
  double offset_time = elapsed_time.InSecondsF() + time_offset_;
  if (offset_time < curve_duration_) {
    scalar_offset = GetPositionAtTime(offset_time) - position_offset_;
    scalar_velocity = GetVelocityAtTime(offset_time);
  } else {
    scalar_offset = GetPositionAtTime(curve_duration_) - position_offset_;
    scalar_velocity = 0;
    still_active = false;
  }

  *offset = gfx::ScaleVector2d(displacement_ratio_, scalar_offset);
  *velocity = gfx::ScaleVector2d(displacement_ratio_, scalar_velocity);
  return still_active;
}

bool FlingCurve::ComputeScrollDeltaAtTime(base::TimeTicks current,
                                          gfx::Vector2dF* delta) {
  DCHECK(delta);
  if (current <= previous_timestamp_) {
    *delta = gfx::Vector2dF();
    return true;
  }

  previous_timestamp_ = current;

  gfx::Vector2dF offset, velocity;
  bool still_active = ComputeScrollOffset(current, &offset, &velocity);

  *delta = offset - cumulative_scroll_;
  cumulative_scroll_ = offset;
  return still_active;
}

gfx::Vector2dF FlingCurve::GetCurveFinalOffset() {
  float scalar_offset = GetPositionAtTime(curve_duration_) - position_offset_;
  return gfx::ScaleVector2d(displacement_ratio_, scalar_offset);
}

}  // namespace ui
