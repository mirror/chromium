// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/binary_data_histogram.h"

#include <cmath>

namespace zucchini {

BinaryDataHistogram::BinaryDataHistogram() {
  histogram_.resize(kNumSlots, 0);
}

BinaryDataHistogram::~BinaryDataHistogram() = default;

void BinaryDataHistogram::Compute(Region region) {
  if (region.size() < sizeof(uint16_t)) {
    is_valid_ = false;
    return;
  }
  // Number of 2-byte intervals fully contained in |region|.
  size_t n = region.size() - sizeof(uint16_t) + 1;
  for (size_t i = 0; i < n; ++i) {
    uint16_t v = Region::at<uint16_t>(region.begin() + i);
    ++histogram_[v];
  }
}

double BinaryDataHistogram::Compare(const BinaryDataHistogram& other) const {
  double ret_sq = 0;
  for (int i = 0; i < kNumSlots; ++i) {
    double d = histogram_[i] - other.histogram_[i];
    ret_sq += d * d;
  }
  return ::sqrt(ret_sq);
}

}  // namespace zucchini
