// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/reliable_bandwidth_estimator.h"

namespace remoting {

ReliableBandwidthEstimator::ReliableBandwidthEstimator(
    double descending_factor,
    double incremental_factor)
    : incremental_factor_(incremental_factor),
      weighted_average_kbps_(descending_factor) {}

ReliableBandwidthEstimator::~ReliableBandwidthEstimator() = default;

double ReliableBandwidthEstimator::EstimatedKbps() const {
  return current_kbps_;
}

void ReliableBandwidthEstimator::Record(double bandwidth_kbps) {
  weighted_average_kbps_.Record(bandwidth_kbps);
  double weighted_kbps = weighted_average_kbps_.WeightedAverage();
  if (current_kbps_ > weighted_kbps) {
    current_kbps_ = weighted_kbps;
  } else {
    current_kbps_ *= incremental_factor_;
    current_kbps_ += weighted_kbps * (1 - incremental_factor_);
  }
}

}  // namespace remoting
