// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/memory/swap_metrics_driver.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

// A SwapMetricsDriver that mocks platform-dependent driver do, but with control
// over when it fails so we can test error conditions.
class MockSwapMetricsDriver : public SwapMetricsDriver {
 public:
  MockSwapMetricsDriver(std::unique_ptr<Delegate> delegate,
                        const base::TimeDelta update_interval,
                        const bool fail_on_update)
      : SwapMetricsDriver(std::move(delegate), update_interval),
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

    delegate_->OnSwapInCount(0, interval);
    delegate_->OnSwapOutCount(0, interval);
    delegate_->OnDecompressedPageCount(0, interval);
    delegate_->OnCompressedPageCount(0, interval);

    return SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess;
  }

 private:
  bool fail_on_update_;
};

// A SwapMetricsDriver::Delegate that counts the number of updates for each
// metric.
class SwapMetricsDelegateCounter : public SwapMetricsDriver::Delegate {
 public:
  SwapMetricsDelegateCounter()
      : num_swaps_in_updates_(0),
        num_swaps_out_updates_(0),
        num_decompressed_updates_(0),
        num_compressed_updates_(0) {}

  ~SwapMetricsDelegateCounter() override = default;

  void OnSwapInCount(uint64_t count, base::TimeDelta interval) override {
    ++num_swaps_in_updates_;
  }

  void OnSwapOutCount(uint64_t count, base::TimeDelta interval) override {
    ++num_swaps_out_updates_;
  }

  void OnDecompressedPageCount(uint64_t count,
                               base::TimeDelta interval) override {
    ++num_decompressed_updates_;
  }

  void OnCompressedPageCount(uint64_t count,
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

  DISALLOW_COPY_AND_ASSIGN(SwapMetricsDelegateCounter);
};

// The time delta between updates must non-zero for the delegate callbacks to be
// invoked.
static const int kUpdateDelayMs = 50;

}  // namespace

class TestSwapMetricsDriver : public testing::Test {
 public:
  static std::unique_ptr<SwapMetricsDriver> CreateDriver(
      const base::TimeDelta update_interval,
      bool fail_on_update) {
    SwapMetricsDelegateCounter* delegate = new SwapMetricsDelegateCounter();
    return base::WrapUnique<SwapMetricsDriver>(new MockSwapMetricsDriver(
        base::WrapUnique<SwapMetricsDriver::Delegate>(delegate),
        update_interval, fail_on_update));
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(TestSwapMetricsDriver, ExpectedMetricCounts) {
  std::unique_ptr<SwapMetricsDriver> driver =
      CreateDriver(base::TimeDelta(), false);

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

  auto* delegate =
      static_cast<SwapMetricsDelegateCounter*>(driver->delegate_.get());

  EXPECT_EQ(2UL, delegate->num_swaps_in_updates());
  EXPECT_EQ(2UL, delegate->num_swaps_out_updates());
  EXPECT_EQ(2UL, delegate->num_decompressed_updates());
  EXPECT_EQ(2UL, delegate->num_compressed_updates());
}

TEST_F(TestSwapMetricsDriver, TimerStartSuccess) {
  std::unique_ptr<SwapMetricsDriver> driver =
      CreateDriver(base::TimeDelta::FromSeconds(60), false);
  auto result = driver->Start();
  EXPECT_EQ(SwapMetricsUpdateResult::kSwapMetricsUpdateSuccess, result);
}

TEST_F(TestSwapMetricsDriver, TimerStartFail) {
  std::unique_ptr<SwapMetricsDriver> driver =
      CreateDriver(base::TimeDelta::FromSeconds(60), true);
  auto result = driver->Start();
  EXPECT_EQ(SwapMetricsUpdateResult::kSwapMetricsUpdateFailed, result);
}

}  // namespace content
