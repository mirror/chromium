// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/swap_thrashing_monitor.h"

#include "base/memory/ptr_util.h"
#include "base/memory/swap_thrashing_delegate.h"
#include "base/test/scoped_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

namespace {

class MockSwapThrashingDelegate : public SwapThrashingDelegate {
 public:
  MockSwapThrashingDelegate()
      : expected_thrashing_level_(
            SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE) {}
  ~MockSwapThrashingDelegate() override {}

  int GetPollingIntervalMs() override { return 1000; }

  SwapThrashingLevel CalculateCurrentSwapThrashingLevel(
      SwapThrashingDelegate::SwapThrashingLevel previous_state) override {
    return expected_thrashing_level_;
  };

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  SwapThrashingLevel expected_thrashing_level_;
};

}  // namespace

// This is outside of the anonymous namespace so that it can be seen as a friend
// to the monitor class.
class TestSwapThrashingMonitor : public SwapThrashingMonitor {
 public:
  explicit TestSwapThrashingMonitor(
      std::unique_ptr<SwapThrashingDelegate> delegate)
      : SwapThrashingMonitor::SwapThrashingMonitor(std::move(delegate)) {}
  ~TestSwapThrashingMonitor() override {}
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

    SwapThrashingMonitor::SetInstance(std::move(monitor));
  }

  void TearDown() override {}

 protected:
  TestSwapThrashingMonitor* monitor_;
  MockSwapThrashingDelegate* delegate_;
};

TEST_F(SwapThrashingMonitorTest, GetInstance) {
  EXPECT_EQ(monitor_, SwapThrashingMonitor::GetInstance());
}

}  // namespace base
