// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEMORY_SWAP_METRICS_OBSERVER_H_
#define CONTENT_BROWSER_MEMORY_SWAP_METRICS_OBSERVER_H_

#include "base/time/time.h"
#include "content/common/content_export.h"

namespace content {

// This class observes the system's swapping behavior, with metrics provided by
// a SwapMetricsDriver.
// Metrics can be platform-specific.
class CONTENT_EXPORT SwapMetricsObserver {
 public:
  virtual ~SwapMetricsObserver();

  virtual void SwapsInPerSecond(double swaps_in_per_sec,
                                base::TimeDelta interval) {}
  virtual void SwapsOutPerSecond(double swaps_out_per_sec,
                                 base::TimeDelta interval) {}
  virtual void DecompressedPagesPerSecond(double decompressed_pages_per_sec,
                                          base::TimeDelta interval) {}
  virtual void CompressedPagesPerSecond(double compressed_pages_per_sec,
                                        base::TimeDelta interval) {}

 protected:
  SwapMetricsObserver();

 private:
  DISALLOW_COPY_AND_ASSIGN(SwapMetricsObserver);
};

}  // namespace

#endif  // CONTENT_BROWSER_MEMORY_SWAP_METRICS_OBSERVER_H_
