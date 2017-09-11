// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/window_occlusion_tracker.h"

#include "base/run_loop.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/test/aura_test_base.h"
#include "ui/aura/test/env_test_helper.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/test/test_windows.h"

namespace aura {

namespace {

class MockWindowDelegate : public test::ColorTestWindowDelegate {
 public:
  MockWindowDelegate() : test::ColorTestWindowDelegate(SK_ColorWHITE) {}

  MOCK_METHOD1(OnWindowOcclusionChanged, void(bool));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockWindowDelegate);
};

class WindowOcclusionTrackerTest : public test::AuraTestBase {
 public:
  WindowOcclusionTrackerTest() = default;

 private:
  test::EnvTestHelper env_test_helper_;

  DISALLOW_COPY_AND_ASSIGN(WindowOcclusionTrackerTest);
};

}  // namespace

TEST_F(WindowOcclusionTrackerTest, WindowCoversWindow) {
  testing::StrictMock<MockWindowDelegate>* delegate_a =
      new testing::StrictMock<MockWindowDelegate>();
  Window* window_a = test::CreateTestWindowWithDelegate(
      delegate_a, 1, gfx::Rect(0, 0, 10, 10), root_window());
  WindowOcclusionTracker::Track(window_a);

  EXPECT_CALL(*delegate_a, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();

  testing::StrictMock<MockWindowDelegate>* delegate_b =
      new testing::StrictMock<MockWindowDelegate>();
  Window* window_b = test::CreateTestWindowWithDelegate(
      delegate_b, 2, gfx::Rect(0, 0, 15, 15), root_window());
  WindowOcclusionTracker::Track(window_b);

  EXPECT_CALL(*delegate_a, OnWindowOcclusionChanged(true));
  EXPECT_CALL(*delegate_b, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();
}

TEST_F(WindowOcclusionTrackerTest, TwoWindowsCoverOneWindow) {
  testing::StrictMock<MockWindowDelegate>* delegate_a =
      new testing::StrictMock<MockWindowDelegate>();
  Window* window_a = test::CreateTestWindowWithDelegate(
      delegate_a, 1, gfx::Rect(0, 0, 10, 10), root_window());
  WindowOcclusionTracker::Track(window_a);

  EXPECT_CALL(*delegate_a, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();

  testing::StrictMock<MockWindowDelegate>* delegate_b =
      new testing::StrictMock<MockWindowDelegate>();
  Window* window_b = test::CreateTestWindowWithDelegate(
      delegate_b, 2, gfx::Rect(0, 0, 5, 10), root_window());
  WindowOcclusionTracker::Track(window_b);

  EXPECT_CALL(*delegate_b, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();
/*
  testing::StrictMock<MockWindowDelegate>* delegate_c =
      new testing::StrictMock<MockWindowDelegate>();
  Window* window_c = test::CreateTestWindowWithDelegate(
      delegate_c, 3, gfx::Rect(5, 0, 5, 10), root_window());
  WindowOcclusionTracker::Track(window_c);

  EXPECT_CALL(*delegate_a, OnWindowOcclusionChanged(true));
  EXPECT_CALL(*delegate_c, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();*/

  LOG(ERROR) << "TEST END";
}

}  // namespace aura
