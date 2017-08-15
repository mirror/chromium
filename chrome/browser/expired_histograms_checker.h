// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXPIRED_HISTOGRAMS_CHECKER_
#define CHROME_BROWSER_EXPIRED_HISTOGRAMS_CHECKER_

#include "base/metrics/histogram_uploading_checker.h"

// ExpiredHistogramsChecker implements HistogramUploadingChecker interface
// to avoid uploading expired metrics.
class ExpiredHistogramsChecker : public base::HistogramUploadingChecker {
 public:
  ExpiredHistogramsChecker();
  ExpiredHistogramsChecker(const uint64_t* array, size_t size);
  ~ExpiredHistogramsChecker();

 private:
  bool filter(uint64_t histogram_hash) const override;
  const uint64_t* array_;
  size_t size_;
};

#endif  // CHROME_BROWSER_EXPIRED_HISTOGRAMS_CHECKER_
