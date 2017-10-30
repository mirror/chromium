// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_RELIABLE_BANDWIDTH_ESTIMATOR_H_
#define REMOTING_PROTOCOL_RELIABLE_BANDWIDTH_ESTIMATOR_H_

#include "remoting/base/descending_samples.h"

namespace remoting {

class ReliableBandwidthEstimator final {
 public:
  explicit ReliableBandwidthEstimator(double descending_factor,
                                      double incremental_factor);
  ~ReliableBandwidthEstimator();

  double EstimatedKbps() const;

  void Record(double bandwidth_kbps);

 private:
  const double incremental_factor_;
  DescendingSamples weighted_average_kbps_;
  double current_kbps_ = 1000000;
};

}  // namespace remoting

#endif  // REMOTING_PROTOCOL_RELIABLE_BANDWIDTH_ESTIMATOR_H_
