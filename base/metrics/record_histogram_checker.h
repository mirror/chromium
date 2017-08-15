// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_METRICS_HISTOGRAM_UPLOADING_CHECKER_
#define BASE_METRICS_HISTOGRAM_UPLOADING_CHECKER_

#include <unordered_set>

#include "base/base_export.h"

namespace base {

// HistogramUploadingChecker provides an interface for checking whether
// the given histogram should be recorded.
class BASE_EXPORT RecordHistogramChecker {
 public:
  bool ShouldRecord(uint64_t histogram_hash) const {
    return filter(histogram_hash);
  }

 private:
  virtual bool filter(uint64_t histogram_hash) const = 0;
};

}  // namespace base

#endif  // BASE_METRICS_HISTOGRAM_UPLOADING_CHECKER_
