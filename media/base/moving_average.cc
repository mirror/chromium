// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/moving_average.h"

#include <cmath>

namespace media {

MovingAverage::MovingAverage(size_t depth) : depth_(depth) {}

MovingAverage::~MovingAverage() {}

void MovingAverage::AddSample(base::TimeDelta sample) {
  if (samples_.size() >= depth_)
    samples_.pop_front();
  samples_.push_back(sample);
  if (sample > max_)
    max_ = sample;
}

base::TimeDelta MovingAverage::Average() const {
  DCHECK(!samples_.empty());
  base::TimeDelta total;
  for (auto s : samples_)
    total += s;
  return total / samples_.size();
}

base::TimeDelta MovingAverage::Deviation() const {
  DCHECK(!samples_.empty());
  const base::TimeDelta average = Average();

  // Perform the calculation in floating point since squaring the delta can
  // exceed the bounds of a uint64_t value given two int64_t inputs.
  double deviation_secs = 0;
  for (auto s : samples_) {
    const double x = (s - average).InSecondsF();
    deviation_secs += x * x;
  }

  deviation_secs /= samples_.size();
  return base::TimeDelta::FromSecondsD(std::sqrt(deviation_secs));
}

void MovingAverage::Reset() {
  max_ = kNoTimestamp;
  samples_.clear();
}

}  // namespace media
