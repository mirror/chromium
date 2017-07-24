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

enum class SwapMetricsUpdateResult {
  kSwapMetricsUpdateSuccess,
  kSwapMetricsUpdateFailed,
};

// This class collects metrics about the system's swapping behavior and provides
// these metrics to an associated delegate. Metrics can be platform-specific.
//
// Periodic updates are driven by |timer_|, and therefore the SwapMetricsDriver
// API is not thread safe. The |timer_| and Delegate methods run on the same
// sequence the driver is started on. The driver should be stopped on the same
// sequence that it is running on.
class CONTENT_EXPORT SwapMetricsDriver {
 public:
  // Delegate class that handles the metrics computed by SwapMetricsDriver.
  class CONTENT_EXPORT Delegate {
   public:
    virtual ~Delegate();

    virtual void OnSwapInCount(uint64_t count, base::TimeDelta interval) {}
    virtual void OnSwapOutCount(uint64_t count, base::TimeDelta interval) {}
    virtual void OnDecompressedPageCount(uint64_t count,
                                         base::TimeDelta interval) {}
    virtual void OnCompressedPageCount(uint64_t count,
                                       base::TimeDelta interval) {}
    virtual void OnUpdateMetricsFailed() {}

   protected:
    Delegate();

   private:
    DISALLOW_COPY_AND_ASSIGN(Delegate);
  };

  virtual ~SwapMetricsDriver();

  // Create a SwapMetricsDriver that will notify the |delegate| when updated
  // metrics are available to consume. If |update_interval| is 0, periodic
  // updating is disabled and metrics should be updated manually via
  // UpdateMetrics().
  // This returns nullptr when swap metrics are not available on the system.
  static std::unique_ptr<SwapMetricsDriver> Create(
      std::unique_ptr<Delegate> delegate,
      const base::TimeDelta update_interval);

  // Initialze swap metrics so updates will start from the values read from this
  // point. If an error occurs while retrieving the platform specific metrics,
  // e.g. I/O error, this returns
  // SwapMetricsUpdateResult::kSwapMetricsUpdateFailed.
  SwapMetricsUpdateResult InitializeMetrics();

  // Returns whether or not the driver is periodically computing metrics.
  // This returns false if the driver has been stopped or never started, or if
  // an error occurred during UpdateMetrics().
  bool IsRunning() const;

  // Starts computing swap metrics every |update_interval_|, which must be > 0.
  // If an error occurs while initializing the metrics, this returns
  // SwapMetricsUpdateResult::kSwapMetricsUpdateFailed. If an error occurs in
  // UpdateMetrics() during a periodic update, the driver will be stopped.
  // IsRunning() can be used to determine if metrics are still being computed.
  SwapMetricsUpdateResult Start();

  // Stop computing swap metrics. To compute metrics for the remaining time
  // since the last update, UpdateMetrics() can be called after Stop().
  void Stop();

  // Update metrics immediately and notify the |delegate_| if metrics were
  // previously updated. UpdateMetrics() must not be called while periodic
  // metrics are being computed.
  // If an error occurs while retrieving the platform specific metrics, e.g. I/O
  // error, this returns SwapMetricsUpdateResult::kSwapMetricsUpdateFailed and
  // the |delegate_|'s OnUpdateMetricsFailed() method will be called.
  SwapMetricsUpdateResult UpdateMetrics();

 protected:
  // |delegate| will receive callbacks when updated metrics are available,
  // either through periodic metrics updates or manually with UpdateMetrics().
  // |delegate| must not be null.
  // If |update_interval| is zero, only manual updates are allowed.
  SwapMetricsDriver(std::unique_ptr<Delegate> delegate,
                    const base::TimeDelta update_interval);

  // Periodically called to update swap metrics.
  void PeriodicUpdateMetrics();

  // Common swap metrics update method for both periodic and manual updates.
  SwapMetricsUpdateResult UpdateMetricsImpl();

  // Platform-dependent parts of UpdateMetricsImpl(). |interval| is the elapsed
  // time since the last UpdateMetricsImpl() call. |interval| will be zero when
  // this function is called for the first time.
  virtual SwapMetricsUpdateResult UpdateMetricsInternal(
      base::TimeDelta interval) = 0;

  // The Delegate observing the metrics updates.
  std::unique_ptr<Delegate> delegate_;

 private:
  FRIEND_TEST_ALL_PREFIXES(TestSwapMetricsDriver, ExpectedMetricCounts);
  FRIEND_TEST_ALL_PREFIXES(TestSwapMetricsDriver, UpdateMetricsFail);

  // The interval between metrics updates.
  base::TimeDelta update_interval_;

  // A periodic timer to update swap metrics.
  base::RepeatingTimer timer_;

  // Holds the last TimeTicks when swap metrics are updated.
  base::TimeTicks last_ticks_;

  DISALLOW_COPY_AND_ASSIGN(SwapMetricsDriver);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEMORY_SWAP_METRICS_DRIVER_H_
