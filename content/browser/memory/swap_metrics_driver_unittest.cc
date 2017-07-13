// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/swap_metrics_driver.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/platform_thread.h"
#include "content/browser/memory/swap_metrics_observer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

// A SwapMetricsDriver that mocks platform-dependent driver do, but with control
// over when it fails so we can test error conditions.
class MockSwapMetricsDriver : public SwapMetricsDriver {
 public:
  MockSwapMetricsDriver(std::unique_ptr<SwapMetricsObserver> observer,
                        const int interval_in_seconds,
                        const bool fail_on_update)
      : SwapMetricsDriver(std::move(observer), interval_in_seconds),
        fail_on_update_(fail_on_update) {}

  bool fail_on_update() const { return fail_on_update_; }

  void SetFailOnUpdate(bool fail_on_update) {
    fail_on_update_ = fail_on_update;
  }

  ~MockSwapMetricsDriver() override = default;

 protected:
  SwapMetricsUpdateResult UpdateMetricsInternal(
      base::TimeDelta interval) override {
    if (fail_on_update_)
      return SwapMetricsUpdateResult::kSwapMetricsUpdateFailed;

    if (interval.is_zero())
      return SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess;

    observer_->SwapsInPerSecond(0.0, interval);
    observer_->SwapsOutPerSecond(0.0, interval);
    observer_->DecompressedPagesPerSecond(0.0, interval);
    observer_->CompressedPagesPerSecond(0.0, interval);

    return SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess;
  }

 private:
  bool fail_on_update_;
};

// A SwapMetricsObserver that counts the number of updates for each metric.
class SwapMetricsObserverCounter : public SwapMetricsObserver {
 public:
  SwapMetricsObserverCounter()
      : num_swaps_in_updates_(0),
        num_swaps_out_updates_(0),
        num_decompressed_updates_(0),
        num_compressed_updates_(0) {}

  ~SwapMetricsObserverCounter() override = default;

  void SwapsInPerSecond(double swaps_in_per_sec,
                        base::TimeDelta interval) override {
    ++num_swaps_in_updates_;
  }

  void SwapsOutPerSecond(double swaps_out_per_sec,
                         base::TimeDelta interval) override {
    ++num_swaps_out_updates_;
  }

  void DecompressedPagesPerSecond(double decompressed_pages_per_sec,
                                  base::TimeDelta interval) override {
    ++num_decompressed_updates_;
  }

  void CompressedPagesPerSecond(double compressed_pages_per_sec,
                                base::TimeDelta interval) override {
    ++num_compressed_updates_;
  }

  size_t num_swaps_in_updates() const { return num_swaps_in_updates_; }
  size_t num_swaps_out_updates() const { return num_swaps_out_updates_; }
  size_t num_decompressed_updates() const { return num_decompressed_updates_; }
  size_t num_compressed_updates() const { return num_compressed_updates_; }

 private:
  size_t num_swaps_in_updates_;
  size_t num_swaps_out_updates_;
  size_t num_decompressed_updates_;
  size_t num_compressed_updates_;

  DISALLOW_COPY_AND_ASSIGN(SwapMetricsObserverCounter);
};

// The time delta between updates must non-zero for the observer callbacks to be
// invoked.
static const int kUpdateDelayMs = 50;

}  // namespace

class TestSwapMetricsDriver : public testing::Test {
 public:
  static std::unique_ptr<SwapMetricsDriver> CreateDriver(
      int interval_in_seconds,
      bool fail_on_update) {
    SwapMetricsObserverCounter* observer = new SwapMetricsObserverCounter();
    return base::WrapUnique<SwapMetricsDriver>(new MockSwapMetricsDriver(
        base::WrapUnique<SwapMetricsObserver>(observer), interval_in_seconds,
        fail_on_update));
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(TestSwapMetricsDriver, ExpectedMetricCounts) {
  std::unique_ptr<SwapMetricsDriver> driver = CreateDriver(0, false);

  auto result = driver->UpdateMetrics();
  EXPECT_EQ(SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess, result);

  base::PlatformThread::Sleep(
      base::TimeDelta::FromMilliseconds(kUpdateDelayMs));
  result = driver->UpdateMetrics();
  EXPECT_EQ(SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess, result);

  base::PlatformThread::Sleep(
      base::TimeDelta::FromMilliseconds(kUpdateDelayMs));
  result = driver->UpdateMetrics();
  EXPECT_EQ(SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess, result);

  auto* observer =
      static_cast<SwapMetricsObserverCounter*>(driver->observer_.get());

  EXPECT_EQ(2UL, observer->num_swaps_in_updates());
  EXPECT_EQ(2UL, observer->num_swaps_out_updates());
  EXPECT_EQ(2UL, observer->num_decompressed_updates());
  EXPECT_EQ(2UL, observer->num_compressed_updates());
}

TEST_F(TestSwapMetricsDriver, TimerStartSuccess) {
  std::unique_ptr<SwapMetricsDriver> driver = CreateDriver(60, false);
  auto result = driver->Start();
  EXPECT_EQ(SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess, result);
}

TEST_F(TestSwapMetricsDriver, TimerStartFail) {
  std::unique_ptr<SwapMetricsDriver> driver = CreateDriver(60, true);
  auto result = driver->Start();
  EXPECT_EQ(SwapMetricsUpdateResult::kSwapMetricsUpdateFailed, result);
}

}  // namespace content
