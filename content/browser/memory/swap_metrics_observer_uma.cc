// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/swap_metrics_observer_uma.h"

#include "base/metrics/histogram_macros.h"

namespace content {

SwapMetricsObserverUma::SwapMetricsObserverUma() = default;

SwapMetricsObserverUma::~SwapMetricsObserverUma() = default;

void SwapMetricsObserverUma::SwapsInPerSecond(double swaps_in_per_sec,
                                              base::TimeDelta interval) {
  UMA_HISTOGRAM_COUNTS_10000("Memory.Experimental.SwapInPerSecond",
                             swaps_in_per_sec);
}

void SwapMetricsObserverUma::SwapsOutPerSecond(double swaps_out_per_sec,
                                               base::TimeDelta interval) {
  UMA_HISTOGRAM_COUNTS_10000("Memory.Experimental.SwapOutPerSecond",
                             swaps_out_per_sec);
}

void SwapMetricsObserverUma::DecompressedPagesPerSecond(
    double decompressed_pages_per_sec,
    base::TimeDelta interval) {
  UMA_HISTOGRAM_COUNTS_10000("Memory.Experimental.DecompressedPagesPerSecond",
                             decompressed_pages_per_sec);
}

void SwapMetricsObserverUma::CompressedPagesPerSecond(
    double compressed_pages_per_sec,
    base::TimeDelta interval) {
  UMA_HISTOGRAM_COUNTS_10000("Memory.Experimental.CompressedPagesPerSecond",
                             compressed_pages_per_sec);
}

}  // namespace content
