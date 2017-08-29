// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/moving_average.h"

#include <algorithm>

namespace media {

MovingAverage::MovingAverage(size_t depth) : depth_(depth), samples_(depth_) {}

MovingAverage::~MovingAverage() {}

void MovingAverage::AddSample(base::TimeDelta sample) {
  // |samples_| is zero-initialized, so |oldest| is also zero before |count_|
  // exceeds |depth_|.
  base::TimeDelta& oldest = samples_[count_++ % depth_];
  total_ += sample - oldest;
  square_sum_secs_ += sample.InSecondsF() * sample.InSecondsF() -
                      oldest.InSecondsF() * oldest.InSecondsF();
  oldest = sample;
  if (sample > max_)
    max_ = sample;
}

base::TimeDelta MovingAverage::Average() const {
  DCHECK_GT(count_, 0u);

  // TODO(dalecurtis): Consider limiting |depth| to powers of two so that we can
  // replace the integer divide with a bit shift operation.

  return total_ / std::min(static_cast<uint64_t>(depth_), count_);
}

base::TimeDelta MovingAverage::Deviation() const {
  DCHECK_GT(count_, 0u);

  const double size = std::min(static_cast<uint64_t>(depth_), count_);
  const double average_secs = total_.InSecondsF() / size;
  double sqr_deviation_secs =
      square_sum_secs_ / size - average_secs * average_secs;
  if (sqr_deviation_secs < 0)
    sqr_deviation_secs = 0;

  return base::TimeDelta::FromSecondsD(sqrt(sqr_deviation_secs));
}

void MovingAverage::Reset() {
  count_ = 0;
  total_ = base::TimeDelta();
  max_ = kNoTimestamp;
  square_sum_secs_ = 0;
  std::fill(samples_.begin(), samples_.end(), base::TimeDelta());
}

}  // namespace media
