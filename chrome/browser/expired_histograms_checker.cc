// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/expired_histograms_checker.h"

#include <algorithm>

#include "chrome/browser/metrics/expired_histograms_array.h"

ExpiredHistogramsChecker::ExpiredHistogramsChecker()
    : ExpiredHistogramsChecker(chrome_metrics::kExpiredHistogramsHashes,
                               chrome_metrics::kNumExpiredHistograms) {}
ExpiredHistogramsChecker::ExpiredHistogramsChecker(const uint64_t* array,
                                                   size_t size) {
  array_ = array;
  size_ = size;
}

bool ExpiredHistogramsChecker::filter(uint64_t histogram_hash) const {
  return std::binary_search(array_, array_ + size_, histogram_hash);
}

ExpiredHistogramsChecker::~ExpiredHistogramsChecker() {}
