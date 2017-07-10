// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEMORY_SWAP_METRICS_OBSERVER_UMA_H_
#define CONTENT_BROWSER_MEMORY_SWAP_METRICS_OBSERVER_UMA_H_

#include "content/browser/memory/swap_metrics_observer.h"

#include "base/time/time.h"

namespace content {

// This class observes system's swapping behavior to collect metrics.
// Metrics can be platform-specific.
class SwapMetricsObserverUma : public SwapMetricsObserver {
 public:
  SwapMetricsObserverUma();
  ~SwapMetricsObserverUma() override;

  void SwapsInPerSecond(double swaps_in_per_sec,
                        base::TimeDelta interval) override;
  void SwapsOutPerSecond(double swaps_out_per_sec,
                         base::TimeDelta interval) override;
  void DecompressedPagesPerSecond(double decompressed_pages_per_sec,
                                  base::TimeDelta interval) override;
  void CompressedPagesPerSecond(double compressed_pages_per_sec,
                                base::TimeDelta interval) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(SwapMetricsObserverUma);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEMORY_SWAP_METRICS_OBSERVER_UMA_H_
