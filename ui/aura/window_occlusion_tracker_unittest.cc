// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/window_occlusion_tracker.h"

#include "base/run_loop.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/env.h"
#include "ui/aura/test/aura_test_base.h"
#include "ui/aura/test/env_test_helper.h"
#include "ui/aura/test/test_window_delegate.h"
#include "ui/aura/test/test_windows.h"

namespace aura {

namespace {

enum class WindowOcclusionChangedExpectation {
  // Expect OnWindowOcclusionChanged() to be called with true as argument.
  OCCLUDED,

  // Expect OnWindowOcclusionChanged() to be called with false as argument.
  NOT_OCCLUDED,

  // Don't expect OnWindowOcclusionChanged() to be called.
  NO_CALL,
};

class MockWindowDelegate : public test::ColorTestWindowDelegate {
 public:
  MockWindowDelegate() : test::ColorTestWindowDelegate(SK_ColorWHITE) {}
  ~MockWindowDelegate() override { EXPECT_FALSE(is_expecting_call()); }

  void set_expectation(WindowOcclusionChangedExpectation expectation) {
    expectation_ = expectation;
  }

  bool is_expecting_call() const {
    return expectation_ != WindowOcclusionChangedExpectation::NO_CALL;
  }

  void OnWindowOcclusionChanged(bool OCCLUDED) override {
    if (expectation_ == WindowOcclusionChangedExpectation::OCCLUDED)
      EXPECT_TRUE(OCCLUDED);
    else if (expectation_ == WindowOcclusionChangedExpectation::NOT_OCCLUDED)
      EXPECT_FALSE(OCCLUDED);
    else
      ADD_FAILURE() << "Unexpected call to OnWindowOcclusionChanged.";
    expectation_ = WindowOcclusionChangedExpectation::NO_CALL;
  }

 private:
  WindowOcclusionChangedExpectation expectation_ =
      WindowOcclusionChangedExpectation::NO_CALL;

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

// This macros calls RunLoop().RunUntilIdle() in the scope of a SCOPED_TRACE to
// ensure that any error that occurs within the RunLoop is attributed to the
// correct line of code.
#define RUN_UNTIL_IDLE()            \
  do {                              \
    SCOPED_TRACE("");               \
    base::RunLoop().RunUntilIdle(); \
  } while (false)

}  // namespace

TEST_F(WindowOcclusionTrackerTest, NonOverlappingWindows) {
  // Create |window_a|. Expect it to be unoccluded.
  MockWindowDelegate* delegate_a = new MockWindowDelegate();
  CreateTestWindow(delegate_a, gfx::Rect(0, 0, 10, 10));
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_a->is_expecting_call());

  // Create |window_b| with bounds that don't overlap |window_a|. Expect it to
  // be unoccluded.
  MockWindowDelegate* delegate_b = new MockWindowDelegate();
  CreateTestWindow(delegate_b, gfx::Rect(15, 0, 10, 10));
  delegate_b->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
}

TEST_F(WindowOcclusionTrackerTest, PartiallyOccludedWindow) {
  // Create |window_a|. Expect it to be unoccluded.
  MockWindowDelegate* delegate_a = new MockWindowDelegate();
  CreateTestWindow(delegate_a, gfx::Rect(0, 0, 10, 10));
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_a->is_expecting_call());

  // Create |window_b| which partially covers |window_a|. Expect both windows to
  // be unoccluded.
  MockWindowDelegate* delegate_b = new MockWindowDelegate();
  CreateTestWindow(delegate_b, gfx::Rect(0, 0, 5, 5));
  delegate_b->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
}

TEST_F(WindowOcclusionTrackerTest, OneWindowOccludesOneWindowAndIsHidden) {
  // Create |window_a|. Expect it to be unoccluded.
  MockWindowDelegate* delegate_a = new MockWindowDelegate();
  CreateTestWindow(delegate_a, gfx::Rect(0, 0, 10, 10));
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_a->is_expecting_call());

  // Create |window_b| with bounds that cover |window_a|. Expect |window_a| to
  // be occluded and |window_b| to be unoccluded.
  MockWindowDelegate* delegate_b = new MockWindowDelegate();
  Window* window_b = CreateTestWindow(delegate_b, gfx::Rect(0, 0, 15, 15));
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::OCCLUDED);
  delegate_b->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_a->is_expecting_call());
  EXPECT_FALSE(delegate_b->is_expecting_call());

  // Hide |window_b|. Expect |window_a| to be unoccluded and |window_b| to be
  // occluded.
  window_b->Hide();
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  delegate_b->set_expectation(WindowOcclusionChangedExpectation::OCCLUDED);
  RUN_UNTIL_IDLE();
}

TEST_F(WindowOcclusionTrackerTest, SemiTransparentWindowCoversWindow) {
  // Create |window_a|. Expect it to be unoccluded.
  MockWindowDelegate* delegate_a = new MockWindowDelegate();
  CreateTestWindow(delegate_a, gfx::Rect(0, 0, 10, 10));
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_a->is_expecting_call());

  // Create |window_b| with bounds that cover |window_a|. Expect |window_a| to
  // be occluded and |window_b| to be unoccluded.
  MockWindowDelegate* delegate_b = new MockWindowDelegate();
  Window* window_b = CreateTestWindow(delegate_b, gfx::Rect(0, 0, 15, 15));
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::OCCLUDED);
  delegate_b->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_a->is_expecting_call());
  EXPECT_FALSE(delegate_b->is_expecting_call());

  // Change the opacity of |window_b| to 0.5f. Expect both windows to be
  // unoccluded.
  window_b->layer()->SetOpacity(0.5f);
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_a->is_expecting_call());

  // Change the opacity of |window_b| back to 1.0f. Expect |window_a| to be
  // occluded.
  window_b->layer()->SetOpacity(1.0f);
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::OCCLUDED);
  RUN_UNTIL_IDLE();
}

TEST_F(WindowOcclusionTrackerTest, TwoWindowsOccludeOneWindow) {
  // Create |window_a|. Expect it to be unoccluded.
  MockWindowDelegate* delegate_a = new MockWindowDelegate();
  CreateTestWindow(delegate_a, gfx::Rect(0, 0, 10, 10));
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_a->is_expecting_call());

  // Create |window_b| with bounds that partially cover |window_a|. Expect both
  // windows to be unoccluded.
  MockWindowDelegate* delegate_b = new MockWindowDelegate();
  CreateTestWindow(delegate_b, gfx::Rect(0, 0, 5, 10));
  EXPECT_FALSE(delegate_b->is_expecting_call());
  delegate_b->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();

  // Create |window_c| with bounds that cover the portion of |window_a| that
  // wasn't already covered by |window_b|. Expect |window_a| to be occluded and
  // other windows to be unoccluded.
  MockWindowDelegate* delegate_c = new MockWindowDelegate();
  CreateTestWindow(delegate_c, gfx::Rect(5, 0, 5, 10));
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::OCCLUDED);
  delegate_c->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
}

TEST_F(WindowOcclusionTrackerTest, ChildDoesNotOccludeParent) {
  // Create |window_a|. Expect it to be non-occluded.
  MockWindowDelegate* delegate_a = new MockWindowDelegate();
  Window* window_a = CreateTestWindow(delegate_a, gfx::Rect(0, 0, 10, 10));
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_a->is_expecting_call());

  // Create |window_b| with |window_a| as parent. The bounds of |window_b| do
  // not fully cover |window_a|. Expect both windows to be unoccluded.
  MockWindowDelegate* delegate_b = new MockWindowDelegate();
  CreateTestWindow(delegate_b, gfx::Rect(0, 0, 5, 5), window_a);
  delegate_b->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
}

TEST_F(WindowOcclusionTrackerTest, ChildOccludesParent) {
  // Create |window_a|. Expect it to be non-occluded.
  MockWindowDelegate* delegate_a = new MockWindowDelegate();
  Window* window_a = CreateTestWindow(delegate_a, gfx::Rect(0, 0, 10, 10));
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_a->is_expecting_call());

  // Create |window_b| with |window_a| as parent. The bounds of |window_b| fuly
  // cover |window_a|. Expect |window_a| to be occluded but not |window_b|.
  MockWindowDelegate* delegate_b = new MockWindowDelegate();
  Window* window_b =
      CreateTestWindow(delegate_b, gfx::Rect(0, 0, 10, 10), window_a);
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::OCCLUDED);
  delegate_b->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_a->is_expecting_call());
  EXPECT_FALSE(delegate_b->is_expecting_call());

  // Create |window_c| whose bounds don't overlap existing windows.
  MockWindowDelegate* delegate_c = new MockWindowDelegate();
  Window* window_c = CreateTestWindow(delegate_c, gfx::Rect(15, 0, 10, 10));
  delegate_c->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_c->is_expecting_call());

  // Change the parent of |window_b| from |window_a| to |window_c|. Expect
  // |window_a| to be unoccluded and |window_c| to be occluded.
  window_a->RemoveChild(window_b);
  window_c->AddChild(window_b);
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  delegate_c->set_expectation(WindowOcclusionChangedExpectation::OCCLUDED);
  RUN_UNTIL_IDLE();
}

TEST_F(WindowOcclusionTrackerTest, StackingChanged) {
  // Create three windows that have the same bounds. Expect window on top of the
  // stack to be unoccluded and other windows to be occluded.
  MockWindowDelegate* delegate_a = new MockWindowDelegate();
  Window* window_a = CreateTestWindow(delegate_a, gfx::Rect(0, 0, 10, 10));
  MockWindowDelegate* delegate_b = new MockWindowDelegate();
  CreateTestWindow(delegate_b, gfx::Rect(0, 0, 10, 10));
  MockWindowDelegate* delegate_c = new MockWindowDelegate();
  CreateTestWindow(delegate_c, gfx::Rect(0, 0, 10, 10));
  delegate_c->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_a->is_expecting_call());
  EXPECT_FALSE(delegate_b->is_expecting_call());
  EXPECT_FALSE(delegate_c->is_expecting_call());

  // Move |window_a| on top of the stack. Expect it to be unoccluded and expect
  // |window_c| to be occluded.
  root_window()->StackChildAtTop(window_a);
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  delegate_c->set_expectation(WindowOcclusionChangedExpectation::OCCLUDED);
  RUN_UNTIL_IDLE();
}

TEST_F(WindowOcclusionTrackerTest, TransparentParentStackingChanged) {
  // Create |window_a| which is transparent. Expect it to be unoccluded.
  MockWindowDelegate* delegate_a = new MockWindowDelegate();
  Window* window_a = CreateTestWindow(delegate_a, gfx::Rect(0, 0, 10, 10));
  window_a->SetTransparent(true);
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_a->is_expecting_call());

  // Create |window_b| which has the same bounds as its parent |window_a|.
  // Expect |window_b| to be unoccluded and |window_a| to be occluded.
  MockWindowDelegate* delegate_b = new MockWindowDelegate();
  CreateTestWindow(delegate_b, gfx::Rect(0, 0, 10, 10), window_a);
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::OCCLUDED);
  delegate_b->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_a->is_expecting_call());
  EXPECT_FALSE(delegate_b->is_expecting_call());

  // Create |window_c| which is transparent and has the same bounds as
  // |window_a| and |window_b|. Expect it to be unoccluded.
  MockWindowDelegate* delegate_c = new MockWindowDelegate();
  Window* window_c = CreateTestWindow(delegate_c, gfx::Rect(0, 0, 10, 10));
  window_c->SetTransparent(true);
  delegate_c->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_b->is_expecting_call());
  EXPECT_FALSE(delegate_c->is_expecting_call());

  // Create |window_d| which has the same bounds as its parent |window_c|.
  // Expect |window_d| to be unoccluded and all other windows to be occluded.
  MockWindowDelegate* delegate_d = new MockWindowDelegate();
  CreateTestWindow(delegate_d, gfx::Rect(0, 0, 10, 10), window_c);
  delegate_b->set_expectation(WindowOcclusionChangedExpectation::OCCLUDED);
  delegate_c->set_expectation(WindowOcclusionChangedExpectation::OCCLUDED);
  delegate_d->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_c->is_expecting_call());
  EXPECT_FALSE(delegate_d->is_expecting_call());

  // Move |window_a| on top of the stack. Expect its child |window_b| to be
  // unoccluded and all other windows to be occluded.
  root_window()->StackChildAtTop(window_a);
  delegate_b->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  delegate_d->set_expectation(WindowOcclusionChangedExpectation::OCCLUDED);
  RUN_UNTIL_IDLE();
}

TEST_F(WindowOcclusionTrackerTest, BoundsChanged) {
  // Create two non-overlapping windows. Expect them to be unoccluded.
  MockWindowDelegate* delegate_a = new MockWindowDelegate();
  Window* window_a = CreateTestWindow(delegate_a, gfx::Rect(0, 0, 10, 10));
  MockWindowDelegate* delegate_b = new MockWindowDelegate();
  Window* window_b = CreateTestWindow(delegate_b, gfx::Rect(0, 10, 10, 10));
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  delegate_b->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_a->is_expecting_call());
  EXPECT_FALSE(delegate_b->is_expecting_call());

  // Move |window_b| on top of |window_a|. Expect |window_a| to be occluded.
  window_b->SetBounds(window_a->bounds());
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::OCCLUDED);
  RUN_UNTIL_IDLE();
}

TEST_F(WindowOcclusionTrackerTest, TransparentParentBoundsChanged) {
  // Create |window_a|. Expect it to be unoccluded.
  MockWindowDelegate* delegate_a = new MockWindowDelegate();
  CreateTestWindow(delegate_a, gfx::Rect(0, 0, 5, 5));
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_a->is_expecting_call());

  // Create |window_b| which doesn't overlap |window_a| and is transparent.
  // Expect it to be unoccluded.
  MockWindowDelegate* delegate_b = new MockWindowDelegate();
  Window* window_b = CreateTestWindow(delegate_b, gfx::Rect(0, 10, 10, 10));
  window_b->SetTransparent(true);
  delegate_b->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_b->is_expecting_call());

  // Create |window_c| which has |window_b| as parent and doesn't occlude any
  // window.
  MockWindowDelegate* delegate_c = new MockWindowDelegate();
  CreateTestWindow(delegate_c, gfx::Rect(0, 0, 5, 5), window_b);
  delegate_c->set_expectation(WindowOcclusionChangedExpectation::NOT_OCCLUDED);
  RUN_UNTIL_IDLE();
  EXPECT_FALSE(delegate_c->is_expecting_call());

  // Move |window_b| so that |windows_b| occlude |window_a|. Expect |window_a|
  // to be occluded and other windows to be unoccluded.
  window_b->SetBounds(gfx::Rect(0, 0, 10, 10));
  delegate_a->set_expectation(WindowOcclusionChangedExpectation::OCCLUDED);
  RUN_UNTIL_IDLE();
}

}  // namespace aura
