// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_BINARY_DATA_HISTOGRAM_H_
#define CHROME_INSTALLER_ZUCCHINI_BINARY_DATA_HISTOGRAM_H_

#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "chrome/installer/zucchini/buffer_view.h"

namespace zucchini {

// A class to compute similarity score between binary data. The heuristic here
// preprocesses input data to a size-65536 histogram, counting the frequency of
// consecutive 2-byte sequences. Therefore data with lengths < 2 are considered
// invalid -- but this is okay for Zucchini's use case. Distance between binary
// data is evaluated as Manhattan (L1) distance between respective histograms.
class BinaryDataHistogram {
 public:
  static constexpr int kNumSlots = 1 << (sizeof(uint16_t) * 8);  // 65536.

  BinaryDataHistogram();
  ~BinaryDataHistogram();

  // Generates the histogram, and updates |is_valid_| and |histogram_|.
  void Compute(ConstBufferView region);

  bool IsValid() const { return is_valid_; }

  // Returns the distance to another histogram. If two binaries are identical
  // then the distance between their histograms is 0. However, the converse does
  // not hold. For example, "aba" and "bab" both have {"ab": 1, "ba": 1}) as
  // their histograms, so their distance will be 0.
  double Distance(const BinaryDataHistogram& other) const;

 private:
  // Binary data with size < 2 are invalid.
  bool is_valid_ = true;
  size_t size_ = 0;

  // Stored histogram, which counts the frequency of 2-byte sequences.
  std::vector<int32_t> histogram_;

  DISALLOW_COPY_AND_ASSIGN(BinaryDataHistogram);
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_BINARY_DATA_HISTOGRAM_H_
