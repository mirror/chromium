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

class MockPageFaultDetectionWindow;

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

  MockPageFaultDetectionWindow* detection_window() {
    EXPECT_NE(nullptr, detection_window_);
    return detection_window_;
  }
  MockPageFaultDetectionWindow* cooldown_window() {
    EXPECT_NE(nullptr, cooldown_window_);
    return cooldown_window_;
  }

  // Returns a page-fault value that will set the hard page-fault rate during
  // the observation to a value that will cause a state change.
  uint64_t ComputeHighThrashingRate(Time next_timestamp);

 private:
  MockPageFaultDetectionWindow* detection_window_;
  MockPageFaultDetectionWindow* cooldown_window_;
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
            0),
        page_fault_observation_override_(0),
        observation_timestamp_override_(Time::Now()),
        invalidated_(false) {}
  ~MockPageFaultDetectionWindow() override {}

  void OnObservation(uint64_t page_fault_observation,
                     Time observation_timestamp) override {
    TestSwapThrashingMonitorDelegateWin::PageFaultDetectionWindow::
        OnObservation(page_fault_observation_override_,
                      observation_timestamp_override_);
  }

  bool IsReady() const override {
    if (invalidated_)
      return false;
    return TestSwapThrashingMonitorDelegateWin::PageFaultDetectionWindow::
        IsReady();
  }

  void EnsureReady() {
    invalidated_ = false;
    window_beginning_ =
        Time::Now() - TimeDelta::FromMilliseconds(window_length_ms_);
  }

  void Invalidate() { invalidated_ = true; }

  void OverrideNextObservation(uint64_t page_fault_observation,
                               Time observation_timestamp) {
    page_fault_observation_override_ = page_fault_observation;
    observation_timestamp_override_ = observation_timestamp;
  }

  Time window_beginning() { return window_beginning_; }
  int window_length_ms() { return window_length_ms_; }

  Time GetEndOfWindowTimestamp() {
    return window_beginning() + TimeDelta::FromMilliseconds(window_length_ms());
  }

 private:
  uint64_t page_fault_observation_override_;
  Time observation_timestamp_override_;
  bool invalidated_;
};

void TestSwapThrashingMonitorDelegateWin::ResetDetectionWindow(
    int window_length_ms,
    uint64_t page_fault_count) {
  std::unique_ptr<MockPageFaultDetectionWindow> detection_window =
      base::MakeUnique<MockPageFaultDetectionWindow>();
  detection_window_ = detection_window.get();
  swap_thrashing_detection_window_.reset(detection_window.release());
}

void TestSwapThrashingMonitorDelegateWin::ResetCooldownWindow(
    int window_length_ms,
    uint64_t page_fault_count) {
  std::unique_ptr<MockPageFaultDetectionWindow> cooldown_window =
      base::MakeUnique<MockPageFaultDetectionWindow>();
  cooldown_window_ = cooldown_window.get();
  swap_thrashing_cooldown_window_.reset(cooldown_window.release());
}

uint64_t TestSwapThrashingMonitorDelegateWin::ComputeHighThrashingRate(
    Time next_timestamp) {
  return SwapThrashingMonitorDelegateWin::kThrashSwappingPageFaultThreshold *
         (next_timestamp - detection_window_->window_beginning()).InSeconds();
}

}  // namespace

class SwapThrashingMonitorDelegateWinTest : public testing::Test {
 public:
  void SetUp() override {}

  void TearDown() override {}
};

TEST_F(SwapThrashingMonitorDelegateWinTest, StateTransition) {
  TestSwapThrashingMonitorDelegateWin swap_thrashing_delegate;

  // We expect the system to initially be in the SWAP_THRASHING_LEVEL_NONE
  // state.
  EXPECT_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE,
            swap_thrashing_delegate.CalculateCurrentSwapThrashingLevel());

  // Reset the detection window to turn it into a mock one.
  swap_thrashing_delegate.ResetDetectionWindow(0, 0);
  MockPageFaultDetectionWindow* detection_window =
      swap_thrashing_delegate.detection_window();
  // Override the next observation with a timestamp that is inferior to the
  // window length.
  Time next_timestamp = detection_window->GetEndOfWindowTimestamp() -
                        TimeDelta::FromMilliseconds(1);
  uint64_t next_thrashing_level = 0;
  detection_window->OverrideNextObservation(next_thrashing_level,
                                            next_timestamp);
  // There should be no state transition as the window hasn't expired yet.
  EXPECT_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE,
            swap_thrashing_delegate.CalculateCurrentSwapThrashingLevel());

  // Force the window to be ready for consultation.
  detection_window->EnsureReady();
  // Use a timestamp equal to the window length so this observation doesn't get
  // ignored.
  next_timestamp = detection_window->GetEndOfWindowTimestamp();
  // Set the thrashing level to a rate that is inferior to the state change
  // threshold.
  next_thrashing_level =
      swap_thrashing_delegate.ComputeHighThrashingRate(next_timestamp) - 1;
  detection_window->OverrideNextObservation(next_thrashing_level,
                                            next_timestamp);
  EXPECT_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_NONE,
            swap_thrashing_delegate.CalculateCurrentSwapThrashingLevel());

  // Set the thrashing rate to the threshold, this should cause a state change.
  next_thrashing_level++;
  detection_window->OverrideNextObservation(next_thrashing_level,
                                            next_timestamp);
  EXPECT_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED,
            swap_thrashing_delegate.CalculateCurrentSwapThrashingLevel());

  // Reset the detection window as it has been invalidated by the state change.
  swap_thrashing_delegate.ResetDetectionWindow(0, 0);
  detection_window = swap_thrashing_delegate.detection_window();
  swap_thrashing_delegate.ResetCooldownWindow(0, 0);
  // Force the detection window to be ready for consultation.
  detection_window->EnsureReady();
  // Invalidate the cooldown window to prevent this from regressing to a lower
  // state.
  swap_thrashing_delegate.cooldown_window()->Invalidate();

  // Use a timestamp just before the end of the window.
  next_timestamp = detection_window->GetEndOfWindowTimestamp() -
                   TimeDelta::FromMilliseconds(1);
  next_thrashing_level = 0;
  detection_window->OverrideNextObservation(next_thrashing_level,
                                            next_timestamp);
  EXPECT_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED,
            swap_thrashing_delegate.CalculateCurrentSwapThrashingLevel());

  // Make the window expire but keep the page-fault rate low, this shouldn't
  // cause a state change.
  next_timestamp = detection_window->GetEndOfWindowTimestamp();
  detection_window->OverrideNextObservation(next_thrashing_level,
                                            next_timestamp);
  EXPECT_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_SUSPECTED,
            swap_thrashing_delegate.CalculateCurrentSwapThrashingLevel());

  // Increase the page-fault rate to induce a change to the confirmed state.
  next_thrashing_level =
      swap_thrashing_delegate.ComputeHighThrashingRate(next_timestamp);
  detection_window->OverrideNextObservation(next_thrashing_level,
                                            next_timestamp);
  EXPECT_EQ(SwapThrashingLevel::SWAP_THRASHING_LEVEL_CONFIRMED,
            swap_thrashing_delegate.CalculateCurrentSwapThrashingLevel());
}

}  // namespace base
