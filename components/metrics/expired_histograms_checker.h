// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_EXPIRED_HISTOGRAMS_CHECKER_H_
#define COMPONENTS_METRICS_EXPIRED_HISTOGRAMS_CHECKER_H_

#include <stdint.h>

#include <unordered_set>

#include "base/macros.h"
#include "base/metrics/record_histogram_checker.h"

namespace metrics {

// ExpiredHistogramsChecker implements RecordHistogramChecker interface
// to avoid recording expired metrics.
class ExpiredHistogramsChecker final : public base::RecordHistogramChecker {
 public:
  // Takes sorted in nondecreasing order array of histogram hashes and its size.
  ExpiredHistogramsChecker(const uint64_t* array, size_t size);
  ~ExpiredHistogramsChecker() override;

  // Checks if the given |histogram_hash| corresponds to an expired histogram.
  bool ShouldRecord(uint64_t histogram_hash) override;

 private:
  // Loads parameters from variation server for enabling/disabling recording
  // particular histograms regardless of their expiry date.
  // Safe for multiple calls.
  void LoadVariationParameters();

  // A sorted array with hashes of expired histograms.
  const uint64_t* const array_;
  // Size of array_.
  const size_t size_;

  // Hashes of the histograms that were affected by the variation parameters.
  std::unordered_set<uint64_t> enabled_histograms_;
  std::unordered_set<uint64_t> disabled_histograms_;

  // True iff variation parameters were loaded.
  bool variation_parameters_loaded_;

  DISALLOW_COPY_AND_ASSIGN(ExpiredHistogramsChecker);
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_EXPIRED_HISTOGRAMS_CHECKER_H_
