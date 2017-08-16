// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXPIRED_HISTOGRAMS_CHECKER_H_
#define CHROME_BROWSER_EXPIRED_HISTOGRAMS_CHECKER_H_

#include "base/macros.h"
#include "base/metrics/record_histogram_checker.h"

// ExpiredHistogramsChecker implements HistogramUploadingChecker interface
// to avoid recording expired metrics.
class ExpiredHistogramsChecker : public base::RecordHistogramChecker {
 public:
  ExpiredHistogramsChecker(const uint64_t* array, size_t size);
  ~ExpiredHistogramsChecker();

 private:
  bool filter(uint64_t histogram_hash) const override;
  const uint64_t* array_;
  size_t size_;

  DISALLOW_COPY_AND_ASSIGN(ExpiredHistogramsChecker);
};

#endif  // CHROME_BROWSER_EXPIRED_HISTOGRAMS_CHECKER_H_
