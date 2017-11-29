// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/metrics/public/cpp/metrics_utils.h"

#include <math.h>

#include <cmath>

namespace ukm {

int64_t GetLowerBucketThreshold(int64_t sample) {
  if (sample <= 0) {
    return 0;
  }

  // The exponential spacing used for buckets when reporting byte usage.
  static const double exp_base = 1.3;
  static const double log_base = std::log(exp_base);

  // This is similar to the bucketing methodology used in histograms, but
  // instead of iteratively calculating each bucket, this calculates the lower
  // end of the specific bucket for network and cached bytes.
  return std::ceil(std::pow(exp_base, std::floor(std::log(sample) / log_base)));
}

}  // namespace ukm
