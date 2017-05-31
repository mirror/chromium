// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/binary_data_histogram.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "zucchini/region.h"

namespace zucchini {

TEST(BinaryDataHistogramTest, Basic) {
  constexpr double kUninitScore = -1;

  unsigned char test_data[] = {2, 137, 42, 0, 0, 0, 7, 11, 1, 11, 255};
  const size_t n = sizeof(test_data);
  Region region(test_data, n);

  std::vector<BinaryDataHistogram> prefix_histograms(n + 1);  // Short to long.
  std::vector<BinaryDataHistogram> suffix_histograms(n + 1);  // Long to short.

  for (size_t i = 0; i <= n; ++i) {
    prefix_histograms[i].Compute(Region(region.begin(), i));
    suffix_histograms[i].Compute(Region(region.begin() + i, n - i));
  }

  // Invalid for and only for sizes 0 and 1.
  EXPECT_FALSE(prefix_histograms[0].IsValid());
  EXPECT_FALSE(prefix_histograms[1].IsValid());
  EXPECT_FALSE(suffix_histograms[n - 1].IsValid());
  EXPECT_FALSE(suffix_histograms[n].IsValid());
  for (size_t i = 2; i <= n; ++i) {
    EXPECT_TRUE(prefix_histograms[i].IsValid());
    EXPECT_TRUE(suffix_histograms[n - i].IsValid());
  }

  // Full-prefix = full-suffix = full data.
  EXPECT_EQ(0.0, prefix_histograms[n].Compare(suffix_histograms[0]));
  EXPECT_EQ(0.0, suffix_histograms[0].Compare(prefix_histograms[n]));

  // Strict prefixes, in increasing size. Compare against full data.
  double prev_prefix_score = kUninitScore;
  for (size_t i = 2; i < n; ++i) {
    double score = prefix_histograms[i].Compare(prefix_histograms[n]);
    // Positivity.
    EXPECT_GT(score, 0.0);
    // Symmetry.
    EXPECT_EQ(score, prefix_histograms[n].Compare(prefix_histograms[i]));
    // Distance should decrease as prefix gets nearer to full data.
    if (prev_prefix_score != kUninitScore) {
      EXPECT_LT(score, prev_prefix_score);
    }
    prev_prefix_score = score;
  }

  // Strict suffixes, in decreasing size. Compare against full data.
  double prev_suffix_score = -1;
  for (size_t i = 1; i <= n - 2; ++i) {
    double score = suffix_histograms[i].Compare(suffix_histograms[0]);
    // Positivity.
    EXPECT_GT(score, 0.0);
    // Symmetry.
    EXPECT_EQ(score, suffix_histograms[0].Compare(suffix_histograms[i]));
    // Distance should increase as suffix gets farther from full data.
    if (prev_suffix_score != kUninitScore) {
      EXPECT_GT(score, prev_suffix_score);
    }
    prev_suffix_score = score;
  }
}

}  // namespace zucchini
