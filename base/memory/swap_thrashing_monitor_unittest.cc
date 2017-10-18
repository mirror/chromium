// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/swap_thrashing_monitor.h"

#include "base/memory/ptr_util.h"
#include "base/memory/swap_thrashing_delegate.h"
#include "base/test/scoped_task_environment.h"
#include "base/timer/mock_timer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

namespace {

using SwapThrashingLevel = SwapThrashingDelegate::SwapThrashingLevel;

class MockSwapThrashingDelegate : public SwapThrashingDelegate {
 public:
  MockSwapThrashingDelegate()
      : expected_thrashing_level_(
            SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE) {}
  ~MockSwapThrashingDelegate() override {}

  int GetPollingIntervalMs() override { return 10000; }

  SwapThrashingLevel CalculateCurrentSwapThrashingLevel(
      SwapThrashingDelegate::SwapThrashingLevel previous_state) override {
    return expected_thrashing_level_;
  };

  void set_expected_thrashing_level(SwapThrashingLevel level) {
    expected_thrashing_level_ = level;
  }

 protected:
  SwapThrashingLevel expected_thrashing_level_;
};

}  // namespace

// This is outside of the anonymous namespace so that it can be seen as a friend
// to the monitor class.
class TestSwapThrashingMonitor : public SwapThrashingMonitor {
 public:
  explicit TestSwapThrashingMonitor(
      std::unique_ptr<MockSwapThrashingDelegate> delegate)
      : SwapThrashingMonitor::SwapThrashingMonitor() {
    // Start by stopping the timer before replacing the delegate by the mock
    // instance.
    timer()->Stop();
    delegate_ = std::move(delegate);
    // Call the |StartObserving| method so the timer get setup to use the mock
    // delegate.
    StartObserving();
  }
  ~TestSwapThrashingMonitor() override {}

  void Release() { SwapThrashingMonitor::ReleaseInstance(); }

  base::RepeatingTimer* timer() { return &timer_; }

  // Set the current swap-thrashing state, used to bypass the state transition
  // logic.
  void set_current_swap_thrashing_level(SwapThrashingLevel level) {
    current_swap_thrashing_level_ = level;
  }
};

class SwapThrashingMonitorTest : public testing::Test {
 public:
  void SetUp() override {
    std::unique_ptr<MockSwapThrashingDelegate> delegate =
        base::MakeUnique<MockSwapThrashingDelegate>();
    delegate_ = delegate.get();

    std::unique_ptr<TestSwapThrashingMonitor> monitor =
        base::MakeUnique<TestSwapThrashingMonitor>(std::move(delegate));
    monitor_ = monitor.get();

    EXPECT_TRUE(monitor_->timer()->IsRunning());

    mock_timer_ = base::MakeUnique<MockTimer>(true, true);
    mock_timer_->Start(FROM_HERE, monitor_->timer()->GetCurrentDelay(),
                       monitor_->timer()->user_task());
    monitor_->timer()->Stop();

    SwapThrashingMonitor::SetInstance(std::move(monitor));
  }

  void TearDown() override { monitor_->Release(); }

 protected:
  TestSwapThrashingMonitor* monitor_;
  MockSwapThrashingDelegate* delegate_;

  std::unique_ptr<MockTimer> mock_timer_;
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(SwapThrashingMonitorTest, GetInstance) {
  EXPECT_EQ(monitor_, SwapThrashingMonitor::GetInstance());
  EXPECT_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE,
            monitor_->GetCurrentSwapThrashingLevel());
}

TEST_F(SwapThrashingMonitorTest, TimerChangeState) {
  EXPECT_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE,
            monitor_->GetCurrentSwapThrashingLevel());

  // Test some valid state transitions.

  delegate_->set_expected_thrashing_level(
      SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED);
  // The thrashing level shouldn't change until the timer gets triggered.
  EXPECT_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE,
            monitor_->GetCurrentSwapThrashingLevel());
  mock_timer_->Fire();
  EXPECT_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED,
            monitor_->GetCurrentSwapThrashingLevel());
  delegate_->set_expected_thrashing_level(
      SwapThrashingLevel::SWAP_THRASHING_LEVEL_CONFIRMED);
  mock_timer_->Fire();
  EXPECT_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_CONFIRMED,
            monitor_->GetCurrentSwapThrashingLevel());
  delegate_->set_expected_thrashing_level(
      SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED);
  mock_timer_->Fire();
  EXPECT_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED,
            monitor_->GetCurrentSwapThrashingLevel());
  delegate_->set_expected_thrashing_level(
      SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE);
  mock_timer_->Fire();
  EXPECT_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE,
            monitor_->GetCurrentSwapThrashingLevel());

  // Test some invalid state transitions.
  delegate_->set_expected_thrashing_level(
      SwapThrashingLevel::SWAP_THRASHING_LEVEL_CONFIRMED);
  mock_timer_->Fire();
  EXPECT_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_INVALID,
            monitor_->GetCurrentSwapThrashingLevel());

  monitor_->set_current_swap_thrashing_level(
      SwapThrashingLevel::SWAP_THRASHING_LEVEL_CONFIRMED);
  delegate_->set_expected_thrashing_level(
      SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE);
  mock_timer_->Fire();
  EXPECT_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_INVALID,
            monitor_->GetCurrentSwapThrashingLevel());
}

}  // namespace base
