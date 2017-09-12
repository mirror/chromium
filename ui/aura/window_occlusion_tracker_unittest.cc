// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/window_occlusion_tracker.h"

#include "base/run_loop.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/env.h"
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

  ~WindowOcclusionTrackerTest() override { base::RunLoop().RunUntilIdle(); }

  Window* CreateTestWindow(WindowDelegate* delegate,
                           gfx::Rect bounds,
                           Window* parent = nullptr) {
    Window* window = test::CreateTestWindowWithDelegate(
        delegate, 1, bounds, parent ? parent : root_window());
    Env::GetInstance()->window_occlusion_tracker()->Track(window);
    return window;
  }

 private:
  test::EnvTestHelper env_test_helper_;

  DISALLOW_COPY_AND_ASSIGN(WindowOcclusionTrackerTest);
};

}  // namespace

TEST_F(WindowOcclusionTrackerTest, NonOverlappingWindows) {
  // Create |window_a|. Expect it to be unoccluded.
  testing::StrictMock<MockWindowDelegate>* delegate_a =
      new testing::StrictMock<MockWindowDelegate>();
  CreateTestWindow(delegate_a, gfx::Rect(0, 0, 10, 10));
  EXPECT_CALL(*delegate_a, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClear(delegate_a);

  // Create |window_b| with bounds that don't overlap |window_a|. Expect it to
  // be unoccluded.
  testing::StrictMock<MockWindowDelegate>* delegate_b =
      new testing::StrictMock<MockWindowDelegate>();
  CreateTestWindow(delegate_b, gfx::Rect(15, 0, 10, 10));
  EXPECT_CALL(*delegate_b, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();
}

TEST_F(WindowOcclusionTrackerTest, PartiallyCoveredWindow) {
  // Create |window_a|. Expect it to be unoccluded.
  testing::StrictMock<MockWindowDelegate>* delegate_a =
      new testing::StrictMock<MockWindowDelegate>();
  CreateTestWindow(delegate_a, gfx::Rect(0, 0, 10, 10));
  EXPECT_CALL(*delegate_a, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClear(delegate_a);

  // Create |window_b| which partially covers |window_a|. Expect both windows to
  // be unoccluded.
  testing::StrictMock<MockWindowDelegate>* delegate_b =
      new testing::StrictMock<MockWindowDelegate>();
  CreateTestWindow(delegate_b, gfx::Rect(0, 0, 5, 5));
  EXPECT_CALL(*delegate_b, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();
}

TEST_F(WindowOcclusionTrackerTest, OneWindowCoversOneWindowAndIsHidden) {
  // Create |window_a|. Expect it to be unoccluded.
  testing::StrictMock<MockWindowDelegate>* delegate_a =
      new testing::StrictMock<MockWindowDelegate>();
  CreateTestWindow(delegate_a, gfx::Rect(0, 0, 10, 10));
  EXPECT_CALL(*delegate_a, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClear(delegate_a);

  // Create |window_b| with bounds that cover |window_a|. Expect |window_a| to
  // be occluded and |window_b| to be unoccluded.
  testing::StrictMock<MockWindowDelegate>* delegate_b =
      new testing::StrictMock<MockWindowDelegate>();
  Window* window_b = CreateTestWindow(delegate_b, gfx::Rect(0, 0, 15, 15));
  EXPECT_CALL(*delegate_a, OnWindowOcclusionChanged(true));
  EXPECT_CALL(*delegate_b, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();

  // Hide |window_b|. Expect |window_a| to be unoccluded and |window_b| to be
  // occluded.
  window_b->Hide();
  EXPECT_CALL(*delegate_a, OnWindowOcclusionChanged(false));
  EXPECT_CALL(*delegate_b, OnWindowOcclusionChanged(true));
  base::RunLoop().RunUntilIdle();
}

TEST_F(WindowOcclusionTrackerTest, TwoWindowsCoverOneWindow) {
  // Create |window_a|. Expect it to be unoccluded.
  testing::StrictMock<MockWindowDelegate>* delegate_a =
      new testing::StrictMock<MockWindowDelegate>();
  CreateTestWindow(delegate_a, gfx::Rect(0, 0, 10, 10));
  EXPECT_CALL(*delegate_a, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClear(delegate_a);

  // Create |window_b| with bounds that partially cover |window_a|. Expect both
  // windows to be unoccluded.
  testing::StrictMock<MockWindowDelegate>* delegate_b =
      new testing::StrictMock<MockWindowDelegate>();
  CreateTestWindow(delegate_b, gfx::Rect(0, 0, 5, 10));
  testing::Mock::VerifyAndClear(delegate_b);
  EXPECT_CALL(*delegate_b, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();

  // Create |window_c| with bounds that cover the portion of |window_a| that
  // isn't already covered by |window_b|. Expect |window_a| to be unoccluded and
  // other windows to be occluded.
  testing::StrictMock<MockWindowDelegate>* delegate_c =
      new testing::StrictMock<MockWindowDelegate>();
  CreateTestWindow(delegate_c, gfx::Rect(5, 0, 5, 10));
  EXPECT_CALL(*delegate_a, OnWindowOcclusionChanged(true));
  EXPECT_CALL(*delegate_c, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();
}

TEST_F(WindowOcclusionTrackerTest, ChildDoesNotCoverParent) {
  // Create |window_a|. Expect it to be non-occluded.
  testing::StrictMock<MockWindowDelegate>* delegate_a =
      new testing::StrictMock<MockWindowDelegate>();
  Window* window_a = CreateTestWindow(delegate_a, gfx::Rect(0, 0, 10, 10));
  EXPECT_CALL(*delegate_a, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClear(delegate_a);

  // Create |window_b| with |window_a| as parent. The bounds of |window_b| do
  // not fully cover |window_a|. Expect both windows to be unoccluded.
  testing::StrictMock<MockWindowDelegate>* delegate_b =
      new testing::StrictMock<MockWindowDelegate>();
  CreateTestWindow(delegate_b, gfx::Rect(0, 0, 5, 5), window_a);
  EXPECT_CALL(*delegate_b, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();
}

TEST_F(WindowOcclusionTrackerTest, ChildCoversParent) {
  // Create |window_a|. Expect it to be non-occluded.
  testing::StrictMock<MockWindowDelegate>* delegate_a =
      new testing::StrictMock<MockWindowDelegate>();
  Window* window_a = CreateTestWindow(delegate_a, gfx::Rect(0, 0, 10, 10));
  EXPECT_CALL(*delegate_a, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClear(delegate_a);

  // Create |window_b| with |window_a| as parent. The bounds of |window_b| fuly
  // cover |window_a|. Expect |window_a| to be occluded but not |window_b|.
  testing::StrictMock<MockWindowDelegate>* delegate_b =
      new testing::StrictMock<MockWindowDelegate>();
  Window* window_b =
      CreateTestWindow(delegate_b, gfx::Rect(0, 0, 10, 10), window_a);
  EXPECT_CALL(*delegate_a, OnWindowOcclusionChanged(true));
  EXPECT_CALL(*delegate_b, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClear(delegate_a);
  testing::Mock::VerifyAndClear(delegate_b);

  // Create |window_c| whose bounds don't overlap existing windows.
  testing::StrictMock<MockWindowDelegate>* delegate_c =
      new testing::StrictMock<MockWindowDelegate>();
  Window* window_c = CreateTestWindow(delegate_c, gfx::Rect(15, 0, 10, 10));
  EXPECT_CALL(*delegate_c, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClear(delegate_c);

  // Change the parent of |window_b| from |window_a| to |window_c|. Expect
  // |window_a| to be unoccluded and |window_c| to be occluded.
  window_a->RemoveChild(window_b);
  window_c->AddChild(window_b);
  EXPECT_CALL(*delegate_a, OnWindowOcclusionChanged(false));
  EXPECT_CALL(*delegate_c, OnWindowOcclusionChanged(true));
  base::RunLoop().RunUntilIdle();
}

TEST_F(WindowOcclusionTrackerTest, StackingChanged) {
  // Create three windows that have the same bounds. Expect window on top of the
  // stack to be unoccluded and other windows to be occluded.
  testing::StrictMock<MockWindowDelegate>* delegate_a =
      new testing::StrictMock<MockWindowDelegate>();
  Window* window_a = CreateTestWindow(delegate_a, gfx::Rect(0, 0, 10, 10));
  testing::StrictMock<MockWindowDelegate>* delegate_b =
      new testing::StrictMock<MockWindowDelegate>();
  CreateTestWindow(delegate_b, gfx::Rect(0, 0, 10, 10));
  testing::StrictMock<MockWindowDelegate>* delegate_c =
      new testing::StrictMock<MockWindowDelegate>();
  CreateTestWindow(delegate_c, gfx::Rect(0, 0, 10, 10));
  EXPECT_CALL(*delegate_c, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClear(delegate_a);
  testing::Mock::VerifyAndClear(delegate_b);
  testing::Mock::VerifyAndClear(delegate_c);

  // Move |window_a| on top of the stack. Expect it to be unoccluded and expect
  // |window_c| to be occluded.
  root_window()->StackChildAtTop(window_a);
  EXPECT_CALL(*delegate_a, OnWindowOcclusionChanged(false));
  EXPECT_CALL(*delegate_c, OnWindowOcclusionChanged(true));
  base::RunLoop().RunUntilIdle();
}

TEST_F(WindowOcclusionTrackerTest, BoundsChanged) {
  // Create two non-overlapping windows. Expect them to be unoccluded.
  testing::StrictMock<MockWindowDelegate>* delegate_a =
      new testing::StrictMock<MockWindowDelegate>();
  Window* window_a = CreateTestWindow(delegate_a, gfx::Rect(0, 0, 10, 10));
  testing::StrictMock<MockWindowDelegate>* delegate_b =
      new testing::StrictMock<MockWindowDelegate>();
  Window* window_b = CreateTestWindow(delegate_b, gfx::Rect(0, 10, 10, 10));
  EXPECT_CALL(*delegate_a, OnWindowOcclusionChanged(false));
  EXPECT_CALL(*delegate_b, OnWindowOcclusionChanged(false));
  base::RunLoop().RunUntilIdle();
  testing::Mock::VerifyAndClear(delegate_a);
  testing::Mock::VerifyAndClear(delegate_b);

  // Move |window_b| on top of |window_a|. Expect |window_a| to be occluded.
  window_b->SetBounds(window_a->bounds());
  EXPECT_CALL(*delegate_a, OnWindowOcclusionChanged(true));
  base::RunLoop().RunUntilIdle();
}

}  // namespace aura
