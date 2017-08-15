// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/binary_data_histogram.h"

#include <algorithm>
#include <cstdlib>

#include "base/logging.h"

namespace zucchini {

BinaryDataHistogram::BinaryDataHistogram() {
  histogram_.resize(kNumSlots, 0);
}

BinaryDataHistogram::~BinaryDataHistogram() = default;

void BinaryDataHistogram::Compute(ConstBufferView region) {
  if (region.size() < sizeof(uint16_t)) {
    is_valid_ = false;
    return;
  }
  // Number of 2-byte intervals fully contained in |region|.
  size_t n = region.size() - sizeof(uint16_t) + 1;
  for (size_t i = 0; i < n; ++i)
    ++histogram_[*reinterpret_cast<const uint16_t*>(region.begin() + i)];
  size_ = n;
}

double BinaryDataHistogram::Distance(const BinaryDataHistogram& other) const {
  double total_diff = 0;
  DCHECK(is_valid_ && other.is_valid_);
  for (int i = 0; i < kNumSlots; ++i) {
    int32_t d = histogram_[i] - other.histogram_[i];
    total_diff += std::abs(d);
  }
  double average_size = (size_ + other.size_) * 0.5;
  return total_diff / average_size;  // Normalize, not checking for divide by 0.
}

}  // namespace zucchini
