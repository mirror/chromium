// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/ozone/evdev/touch_filter/inconsistent_speed_filter.h"

#include <inttypes.h>
#include <algorithm>
#include <cmath>

#include "base/logging.h"
#include "ui/events/ozone/evdev/touch_evdev_types.h"

namespace ui {

namespace {

// More or less a copy of std::clamp from C++17
template <typename T>
const T& clamp(const T& value, const T& low, const T& high) {
  assert(low < high);
  return value > high ? high : (value < low ? low : value);
}

// Maximum multiplication factor that drift correction can influence pen speed.
static constexpr float kDriftMaxCorrectionFactor = 0.9f;

// Drift amount at which the maximum correction factor should be reached.
static constexpr float kMaxCorrectionDriftAmount = 100f;

// Minimum delta between output timestamps.
static constexpr float kMinDeltaT = 1.0f;
// Maxmumum delta between output timestamps.
static constexpr float kMaxDeltaT = 16.0f;

// Minimum delta between input timestamps at which we should reset the filter.
static constexpr float kResetThreshold = 16.0f;

}  // namespace

InconsistentSpeedFilter::MovingAverageWindow::MovingAverageWindow(
    size_t kernel_size)
    : kernel_size_(kernel_size) {}
InconsistentSpeedFilter::MovingAverageWindow::~MovingAverageWindow() {}

void InconsistentSpeedFilter::MovingAverageWindow::Add(float value) {
  buffer_.push_front(value);
  while (buffer_.size() > kernel_size_) {
    buffer_.pop_back();
  }
}

void InconsistentSpeedFilter::MovingAverageWindow::Clear() {
  buffer_.clear();
}

float InconsistentSpeedFilter::MovingAverageWindow::Average() {
  return std::accumulate(buffer_.begin(), buffer_.end(), 0.0f) / buffer_.size();
}

InconsistentSpeedFilter::InconsistentSpeedFilter(size_t kernel_size)
    : velocity_window_(kernel_size), drift_window_(kernel_size) {}

void InconsistentSpeedFilter::Reset() {
  previous_t_sensor_ = base::TimeTicks();
  previous_t_corrected_ = base::TimeTicks();
  previous_x_ = 0;
  previous_y_ = 0;

  velocity_window_.Clear();
  drift_window_.Clear();
}

float InconsistentSpeedFilter::CalculateCorrectedDeltaT(float dt_sensor,
                                                        float dxy,
                                                        float v_sensor) {
  // Recalculating the timestamp purely based on mean velocity will inevitably
  // cause us to drift away from the real timestamps. We increase/decrease the
  // target velocity to account for the drift.
  float drift_mean = drift_window_.Average();
  float drift_correction = 1.0f + (drift_mean / kMaxCorrectionDriftAmount *
                                   kDriftMaxCorrectionFactor);
  drift_correction = clamp(drift_correction, 1.0f - kDriftMaxCorrectionFactor,
                           1.0f + kDriftMaxCorrectionFactor);

  float v_target = velocity_window_.Average() / drift_correction;
  float dt_corrected = dxy / v_target;
  dt_corrected = clamp(dt_corrected, kMinDeltaT, kMaxDeltaT);
  return std::isnan(dt_corrected) ? dt_sensor : dt_corrected;
}

base::TimeTicks InconsistentSpeedFilter::FilterTimestamp(
    base::TimeTicks t_sensor,
    const InProgressTouchEvdev& event) {
  // Don't apply filter while hovering.
  if (!event.touching && !event.was_touching) {
    Reset();
    return t_sensor;
  }

  base::TimeTicks t_corrected = t_sensor;
  if (!previous_t_sensor_.is_null()) {
    float dt_sensor = (t_sensor - previous_t_sensor_).InMillisecondsF();
    if (dt_sensor > kResetThreshold) {
      Reset();
      return t_sensor;
    }

    float dx = event.x - previous_x_;
    float dy = event.y - previous_y_;
    float dxy = sqrtf(dx * dx + dy * dy);
    float v_sensor = dxy / dt_sensor;
    velocity_window_.Add(v_sensor);

    float dt_corrected = CalculateCorrectedDeltaT(dt_sensor, dxy, v_sensor);
    t_corrected = previous_t_corrected_ +
                  base::TimeDelta::FromMillisecondsD(dt_corrected);

    float drift = (t_sensor - t_corrected).InMillisecondsF();
    drift_window_.Add(drift);
  }

  previous_t_sensor_ = t_sensor;
  previous_t_corrected_ = t_corrected;
  previous_x_ = event.x;
  previous_y_ = event.y;
  return t_corrected;
}

}  // namespace ui
