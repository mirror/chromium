// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_BASE_DESCENDING_SAMPLES_H_
#define REMOTING_BASE_DESCENDING_SAMPLES_H_

#include <cstdint>

namespace remoting {

// Aggregates the samples and gives each of them a weight based on its age.
class DescendingSamples final {
 public:
  explicit DescendingSamples(double weight_factor);
  ~DescendingSamples();

  void Record(double value);

  double WeightedAverage() const;

 private:
  const double weight_factor_;
  int64_t count_ = 0;
  double weighted_sum_ = 0;
};

}  // namespace remoting

#endif  // REMOTING_BASE_DESCENDING_SAMPLES_H_
