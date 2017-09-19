// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/widget.h"

#include <memory>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/run_loop.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/browser/ui/views/fullscreen_control/fullscreen_control_popup.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/views/test/widget_test.h"

class FullscreenControlPopupTest : public views::test::WidgetTest {
 public:
  // views::test::WidgetTest:
  void SetUp() override;
  void TearDown() override;

 protected:
  void RunAnimationFor(base::TimeDelta duration);
  void CompleteAnimation();

  gfx::Rect GetPopupBounds() const;

  std::unique_ptr<FullscreenControlPopup> popup_;

 private:
  void RunUntilIdle();

  std::unique_ptr<gfx::AnimationTestApi> animation_api_;
  views::Widget* parent_widget_;
};

void FullscreenControlPopupTest::SetUp() {
  views::test::WidgetTest::SetUp();
  parent_widget_ = CreateTopLevelFramelessPlatformWidget();
  popup_ = std::make_unique<FullscreenControlPopup>(
      parent_widget_->GetNativeView(), base::BindRepeating(&base::DoNothing));
  animation_api_ =
      std::make_unique<gfx::AnimationTestApi>(popup_->animation_for_testing());
}

void FullscreenControlPopupTest::TearDown() {
  parent_widget_->Close();
  views::test::WidgetTest::TearDown();
}

void FullscreenControlPopupTest::RunAnimationFor(base::TimeDelta duration) {
  base::TimeTicks now = base::TimeTicks::Now();
  animation_api_->SetStartTime(now);
  animation_api_->Step(now + duration);
}

void FullscreenControlPopupTest::CompleteAnimation() {
  RunAnimationFor(base::TimeDelta::FromMilliseconds(5000));
}

gfx::Rect FullscreenControlPopupTest::GetPopupBounds() const {
  return popup_->widget()->GetClientAreaBoundsInScreen();
}

TEST_F(FullscreenControlPopupTest, ShowPopupAnimated) {
  EXPECT_FALSE(popup_->IsAnimating());
  EXPECT_FALSE(popup_->IsVisible());

  gfx::Rect bounds = {0, 0, 640, 480};
  popup_->Show(bounds);
  EXPECT_TRUE(popup_->IsAnimating());
  EXPECT_TRUE(popup_->IsVisible());

  // The popup should be above the screen when the animation is just started.
  EXPECT_LT(GetPopupBounds().y(), 0);

  gfx::Rect final_bounds = popup_->GetFinalBounds();
  EXPECT_GT(final_bounds.y(), 0);
  CompleteAnimation();

  EXPECT_FALSE(popup_->IsAnimating());
  EXPECT_TRUE(popup_->IsVisible());
  EXPECT_EQ(GetPopupBounds(), final_bounds);
}

TEST_F(FullscreenControlPopupTest, HidePopupWhileStillShowing) {
  gfx::Rect bounds = {0, 0, 640, 480};
  popup_->Show(bounds);

  RunAnimationFor(base::TimeDelta::FromMilliseconds(50));

  EXPECT_TRUE(popup_->IsAnimating());
  EXPECT_TRUE(popup_->IsVisible());

  // The popup is partially shown.
  EXPECT_GT(GetPopupBounds().bottom(), 0);

  popup_->Hide(true);
  EXPECT_TRUE(popup_->IsAnimating());
  EXPECT_TRUE(popup_->IsVisible());
  EXPECT_GT(GetPopupBounds().bottom(), 0);

  CompleteAnimation();

  EXPECT_FALSE(popup_->IsAnimating());
  EXPECT_FALSE(popup_->IsVisible());
  EXPECT_LT(GetPopupBounds().y(), 0);
}

TEST_F(FullscreenControlPopupTest, HidePopupWithoutAnimation) {
  gfx::Rect bounds = {0, 0, 640, 480};
  popup_->Show(bounds);

  CompleteAnimation();

  popup_->Hide(false);
  EXPECT_FALSE(popup_->IsAnimating());
  EXPECT_FALSE(popup_->IsVisible());
}
