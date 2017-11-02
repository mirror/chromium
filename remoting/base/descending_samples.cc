// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/base/descending_samples.h"

#include <cmath>

namespace remoting {

DescendingSamples::DescendingSamples(double weight_factor)
    : weight_factor_(weight_factor) {}
DescendingSamples::~DescendingSamples() = default;

void DescendingSamples::Record(double value) {
  weighted_sum_ *= weight_factor_;
  weighted_sum_ += value;
  count_++;
}

double DescendingSamples::WeightedAverage() const {
  if (count_ == 0) {
    return 0;
  }
  return weighted_sum_ * (1 - weight_factor_) /
         (1 - pow(weight_factor_, count_));
}

}  // namespace remoting
