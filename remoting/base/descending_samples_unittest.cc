// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/base/descending_samples.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {

TEST(DescendingSamplesTest, CalculateWeightedAverage) {
  static constexpr double kWeightFactor = 0.9;
  DescendingSamples samples(kWeightFactor);
  double numerator = 0;
  double denominator = 0;
  for (int i = 0; i < 100; i++) {
    samples.Record(i);
    numerator *= kWeightFactor;
    numerator += i;
    denominator *= kWeightFactor;
    denominator++;
  }
  EXPECT_EQ(samples.WeightedAverage(), numerator / denominator);
}

TEST(DescendingSamplesTest, CalculateWeightedAverage_SameValues) {
  DescendingSamples samples(0.9);
  for (int i = 0; i < 100; i++) {
    samples.Record(100);
  }
  EXPECT_EQ(samples.WeightedAverage(), 100);
}

TEST(DescendingSamplesTest, ReturnZeroIfNoRecords) {
  DescendingSamples samples(0.9);
  EXPECT_EQ(samples.WeightedAverage(), 0);
}

}  // namespace remoting
