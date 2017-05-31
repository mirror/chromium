// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ZUCCHINI_BINARY_DATA_HISTOGRAM_H_
#define ZUCCHINI_BINARY_DATA_HISTOGRAM_H_

#include <cstdint>
#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "zucchini/region.h"

namespace zucchini {

// A class to compute similarity between binary data. Our heuristic preprocesses
// input data to a size-65536 histogram, counting the frequency of conscutive
// 2-byte sequences. Therefore data with lengths < 2 are considered invalid --
// but this is okay for Zucchini's use case. Distance between binary data is
// evaluated as the Euclidian (L2) distance between respective histograms.
class BinaryDataHistogram {
 public:
  static constexpr int kNumSlots = 1 << (sizeof(uint16_t) * 8);  // 65536.

  BinaryDataHistogram();
  ~BinaryDataHistogram();

  // Generates the histogram, and updates |is_valid_| and |histogram_|.
  void Compute(Region region);

  bool IsValid() const { return is_valid_; }

  // Returns the distance to another histogram, using the Euclidian distance.
  // Identical binary data have distance of 0, but the converse does not hold
  // (e.g.: "aba" and "bab" have identical histograms {"ab": 1, "ba": 1}).
  double Compare(const BinaryDataHistogram& other) const;

 private:
  // Binary data with size < 2 are invalid.
  bool is_valid_ = true;

  // Stored histogram, which counts the frequency of 2-byte sequences.
  std::vector<int32_t> histogram_;

  DISALLOW_COPY_AND_ASSIGN(BinaryDataHistogram);
};

}  // namespace zucchini

#endif  // ZUCCHINI_BINARY_DATA_HISTOGRAM_H_
