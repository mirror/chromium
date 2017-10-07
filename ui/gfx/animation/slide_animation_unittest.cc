// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/animation/slide_animation.h"

#include <memory>

#include "base/macros.h"
#include "base/test/scoped_task_environment.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/gfx/animation/test_animation_delegate.h"

namespace gfx {

////////////////////////////////////////////////////////////////////////////////
// SlideAnimationTest
class SlideAnimationTest : public testing::Test {
 public:
  void RunAnimationFor(base::TimeDelta duration) {
    base::TimeTicks now = base::TimeTicks::Now();
    animation_api_->SetStartTime(now);
    animation_api_->Step(now + duration);
  }

  std::unique_ptr<AnimationTestApi> animation_api_;
  std::unique_ptr<SlideAnimation> slide_animation_;

 protected:
  SlideAnimationTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {
    slide_animation_.reset(new SlideAnimation(nullptr));
    animation_api_.reset(new AnimationTestApi(slide_animation_.get()));
  }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

// Tests animation construction.
TEST_F(SlideAnimationTest, InitialState) {
  SlideAnimation animation(nullptr);
  // By default, slide animations are 60 Hz, so the timer interval should be
  // 1/60th of a second.
  EXPECT_EQ(1000 / 60, animation.timer_interval().InMilliseconds());
  // Duration defaults to 120 ms.
  EXPECT_EQ(120, animation.GetSlideDuration());
  // Slide is neither showing nor closing.
  EXPECT_FALSE(animation.IsShowing());
  EXPECT_FALSE(animation.IsClosing());
  // Starts at 0.
  EXPECT_EQ(0.0, animation.GetCurrentValue());
}

TEST_F(SlideAnimationTest, Basics) {
  SlideAnimation animation(nullptr);
  AnimationTestApi test_api(&animation);

  // Use linear tweening to make the math easier below.
  animation.SetTweenType(Tween::LINEAR);

  // Duration can be set after construction.
  animation.SetSlideDuration(100);
  EXPECT_EQ(100, animation.GetSlideDuration());

  // Show toggles the appropriate state.
  animation.Show();
  EXPECT_TRUE(animation.IsShowing());
  EXPECT_FALSE(animation.IsClosing());

  // Simulate running the animation.
  test_api.SetStartTime(base::TimeTicks());
  test_api.Step(base::TimeTicks() + base::TimeDelta::FromMilliseconds(50));
  EXPECT_EQ(0.5, animation.GetCurrentValue());

  // We can start hiding mid-way through the animation.
  animation.Hide();
  EXPECT_FALSE(animation.IsShowing());
  EXPECT_TRUE(animation.IsClosing());

  // Reset stops the animation.
  animation.Reset();
  EXPECT_EQ(0.0, animation.GetCurrentValue());
  EXPECT_FALSE(animation.IsShowing());
  EXPECT_FALSE(animation.IsClosing());
}

// Tests that delegate is not notified when animation is running and is deleted.
// (Such a scenario would cause problems for BoundsAnimator).
TEST_F(SlideAnimationTest, DontNotifyOnDelete) {
  TestAnimationDelegate delegate;
  std::unique_ptr<SlideAnimation> animation(new SlideAnimation(&delegate));

  // Start the animation.
  animation->Show();

  // Delete the animation.
  animation.reset();

  // Make sure the delegate wasn't notified.
  EXPECT_FALSE(delegate.finished());
  EXPECT_FALSE(delegate.canceled());
}

// Tests that animations which are started partway and have a dampening factor
// of 1 progress properly.
TEST_F(SlideAnimationTest,
       AnimationWithPartialProgressAndDefaultDampeningFactor) {
  slide_animation_->SetTweenType(Tween::LINEAR);
  const double duration = 100;
  slide_animation_->SetSlideDuration(duration);
  slide_animation_->Show();
  // Advance the animation to halfway done.
  RunAnimationFor(base::TimeDelta::FromMilliseconds(duration / 2.0));
  EXPECT_EQ(0.5, slide_animation_->GetCurrentValue());
  // Reverse the animation.
  slide_animation_->Hide();

  // Run for 10ms, the animation should step to 0.4.
  RunAnimationFor(base::TimeDelta::FromMilliseconds(10));

  EXPECT_EQ(0.4, slide_animation_->GetCurrentValue());
}

// Tests that animations which are started partway and have a dampening factor
// of >1 progress properly.
TEST_F(SlideAnimationTest,
       AnimationWithPartialProgressAndNonDefaultDampeningFactor) {
  slide_animation_->SetTweenType(Tween::LINEAR);
  const double duration = 100;
  slide_animation_->SetDampeningValue(3.0);
  slide_animation_->SetSlideDuration(duration);
  slide_animation_->Show();
  // Advance the animation to halfway done.
  RunAnimationFor(base::TimeDelta::FromMilliseconds(duration / 2.0));
  EXPECT_EQ(0.5, slide_animation_->GetCurrentValue());
  // Reverse the animation.
  slide_animation_->Hide();

  // Run the animation for 8ms.
  RunAnimationFor(base::TimeDelta::FromMilliseconds(8));

  EXPECT_FLOAT_EQ(0.25999039, slide_animation_->GetCurrentValue());
}

}  // namespace gfx
