// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_BINARY_DATA_HISTOGRAM_H_
#define CHROME_INSTALLER_ZUCCHINI_BINARY_DATA_HISTOGRAM_H_

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "base/macros.h"
#include "chrome/installer/zucchini/buffer_view.h"

namespace zucchini {

// A class to compute similarity score between binary data. The heuristic here
// preprocesses input data to a size-65536 histogram, counting the frequency of
// consecutive 2-byte sequences. Therefore data with lengths < 2 are considered
// invalid -- but this is okay for Zucchini's use case.
class BinaryDataHistogram {
 public:
  static constexpr int kNumBins = 1 << (sizeof(uint16_t) * 8);  // 65536.

  BinaryDataHistogram();

  ~BinaryDataHistogram();

  // Attempts to compute the histogram, returns true iff successful.
  bool Compute(ConstBufferView region);

  bool IsValid() const { return !histogram_.empty(); }

  // Returns distance to another histogram (heuristics). If two binaries are
  // identical then their histogram distance is 0. However, the converse is not
  // true in general. For example, "aba" and "bab" are different, but their
  // histogram distance is 0 (both histograms are {"ab": 1, "ba": 1}).
  double Distance(const BinaryDataHistogram& other) const;

 private:
  // Size of the |region| used to create the histogram.
  size_t size_ = 0;

  // Stored histogram, which counts the frequency of 2-byte sequences. Storing
  // as signed integer to simplifiy histogram distance computation.
  std::vector<int32_t> histogram_;

  DISALLOW_COPY_AND_ASSIGN(BinaryDataHistogram);
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_BINARY_DATA_HISTOGRAM_H_
