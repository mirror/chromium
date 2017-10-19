// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/swap_thrashing_delegate_win.h"

#include "base/memory/ptr_util.h"
#include "base/test/scoped_task_environment.h"
#include "base/timer/mock_timer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

namespace {

using SwapThrashingLevel = SwapThrashingMonitorDelegateWin::SwapThrashingLevel;

class TestSwapThrashingMonitorDelegateWin
    : public SwapThrashingMonitorDelegateWin {
 public:
  using PageFaultDetectionWindow =
      SwapThrashingMonitorDelegateWin::PageFaultDetectionWindow;
  explicit TestSwapThrashingMonitorDelegateWin()
      : SwapThrashingMonitorDelegateWin::SwapThrashingMonitorDelegateWin() {}
  ~TestSwapThrashingMonitorDelegateWin() override {}

  void ResetDetectionWindow(int window_length_ms,
                            uint64_t page_fault_count) override;
  void ResetCooldownWindow(int window_length_ms,
                           uint64_t page_fault_count) override;
};

// A mock for the PageFaultDetectionWindow class.
//
// Used to induce state transitions in TestSwapThrashingMonitorDelegateWin.
class MockPageFaultDetectionWindow
    : public TestSwapThrashingMonitorDelegateWin::PageFaultDetectionWindow {
 public:
  static const int kDefaultWindowLengthMs = 1000;

  explicit MockPageFaultDetectionWindow()
      : TestSwapThrashingMonitorDelegateWin::PageFaultDetectionWindow(
            kDefaultWindowLengthMs,
            0) {}
  ~MockPageFaultDetectionWindow() override {}
};

void TestSwapThrashingMonitorDelegateWin::ResetDetectionWindow(
    int window_length_ms,
    uint64_t page_fault_count) {
  swap_thrashing_detection_window_ =
      base::MakeUnique<MockPageFaultDetectionWindow>();
}

void TestSwapThrashingMonitorDelegateWin::ResetCooldownWindow(
    int window_length_ms,
    uint64_t page_fault_count) {
  swap_thrashing_cooldown_window_ =
      base::MakeUnique<MockPageFaultDetectionWindow>();
}

}  // namespace

class SwapThrashingMonitorDelegateWinTest : public testing::Test {
 public:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(SwapThrashingMonitorDelegateWinTest, CalculatePageFaultRate) {
  TestSwapThrashingMonitorDelegateWin swap_thrashing_delegate;
  EXPECT_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE,
            swap_thrashing_delegate.CalculateCurrentSwapThrashingLevel());
}

}  // namespace base
