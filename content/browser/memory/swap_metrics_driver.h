// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEMORY_SWAP_METRICS_DRIVER_H_
#define CONTENT_BROWSER_MEMORY_SWAP_METRICS_DRIVER_H_

#include <memory>

#include "base/gtest_prod_util.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/common/content_export.h"

namespace content {

class SwapMetricsObserver;

enum class SwapMetricsUpdateResult {
  kSwapMetricsUpdateSuccess,
  kSwapMetricsUpdateFailed,
};

// This class collects metrics about the system's swapping behavior and provides
// these metrics to an associated observer.
// Metrics can be platform-specific.
class CONTENT_EXPORT SwapMetricsDriver {
 public:
  virtual ~SwapMetricsDriver();

  // Create a SwapMetricsDriver that will notify the |observer| when updated
  // metrics are available to consume. If |interval_in_seconds| is 0, periodic
  // updating is disabled and metrics should be updated manually via
  // UpdateMetrics().
  // This returns nullptr when swap metrics are not available on the system.
  static std::unique_ptr<SwapMetricsDriver> Create(
      std::unique_ptr<SwapMetricsObserver> observer,
      const int interval_in_seconds = 0);

  // Initialze swap metrics so updates will start from the values read from this
  // point.
  SwapMetricsUpdateResult InitializeMetrics();

  // Starts computing swap metrics every |update_interval_| seconds.
  // |update_interval_| seconds must be > 0.
  SwapMetricsUpdateResult Start();

  // Stop computing swap metrics. UpdateMetrics() can be called after stopping
  // to compute metrics for the remaining time since the last update.
  void Stop();

  // Update metrics immediately and notify the |observer_| if metrics were
  // previously updated.
  // UpdateMetrics() must not be called while periodic metrics are being
  // computed.
  SwapMetricsUpdateResult UpdateMetrics();

 protected:
  // |observer| will receive callbacks when updated metrics are available,
  // either through periodic metrics updates or manually with UpdateMetrics().
  // |observer| must not be null.
  // If |interval_in_seconds| is zero, only manual updates are allowed.
  SwapMetricsDriver(std::unique_ptr<SwapMetricsObserver> observer,
                    const int interval_in_seconds);

  // Periodically called to update swap metrics.
  void PeriodicUpdateMetrics();

  // Common swap metrics update method for both periodic and manual updates.
  SwapMetricsUpdateResult UpdateMetricsImpl();

  // Platform-dependent parts of UpdateMetricsImpl(). |interval| is the elapsed
  // time since the last UpdateMetricsImpl() call. |interval| will be zero when
  // this function is called for the first time.
  virtual SwapMetricsUpdateResult UpdateMetricsInternal(
      base::TimeDelta interval) = 0;

  // The SwapMetricsObserver observing the metrics updates.
  std::unique_ptr<SwapMetricsObserver> observer_;

 private:
  FRIEND_TEST_ALL_PREFIXES(TestSwapMetricsDriver, ExpectedMetricCounts);

  // The interval between metrics updates.
  base::TimeDelta update_interval_;

  // A periodic timer to update swap metrics.
  base::RepeatingTimer timer_;

  // Holds the last TimeTicks when swap metrics are updated.
  base::TimeTicks last_ticks_;

  DISALLOW_COPY_AND_ASSIGN(SwapMetricsDriver);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEMORY_SWAP_METRICS_OBSERVER_H_
