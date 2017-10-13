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
  // By default, slide animations are 60 Hz, so the timer interval should be
  // 1/60th of a second.
  EXPECT_EQ(1000 / 60, slide_animation_->timer_interval().InMilliseconds());
  // Duration defaults to 120 ms.
  EXPECT_EQ(120, slide_animation_->GetSlideDuration());
  // Slide is neither showing nor closing.
  EXPECT_FALSE(slide_animation_->IsShowing());
  EXPECT_FALSE(slide_animation_->IsClosing());
  // Starts at 0.
  EXPECT_EQ(0.0, slide_animation_->GetCurrentValue());
}

TEST_F(SlideAnimationTest, Basics) {
  // Use linear tweening to make the math easier below.
  slide_animation_->SetTweenType(Tween::LINEAR);

  // Duration can be set after construction.
  slide_animation_->SetSlideDuration(100);
  EXPECT_EQ(100, slide_animation_->GetSlideDuration());

  // Show toggles the appropriate state.
  slide_animation_->Show();
  EXPECT_TRUE(slide_animation_->IsShowing());
  EXPECT_FALSE(slide_animation_->IsClosing());

  // Simulate running the animation.
  base::TimeTicks now = base::TimeTicks::Now();
  animation_api_->SetStartTime(now);
  animation_api_->Step(now + base::TimeDelta::FromMilliseconds(50));
  // RunAnimationFor(base::TimeDelta::FromMilliseconds(50));
  EXPECT_EQ(0.5, slide_animation_->GetCurrentValue());

  // We can start hiding mid-way through the animation.
  slide_animation_->Hide();
  EXPECT_FALSE(slide_animation_->IsShowing());
  EXPECT_TRUE(slide_animation_->IsClosing());

  // Reset stops the animation.
  slide_animation_->Reset();
  EXPECT_EQ(0.0, slide_animation_->GetCurrentValue());
  EXPECT_FALSE(slide_animation_->IsShowing());
  EXPECT_FALSE(slide_animation_->IsClosing());
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
  EXPECT_EQ(slide_animation_->GetCurrentValue(), 0.0);

  // Advance the animation to halfway done.
  RunAnimationFor(base::TimeDelta::FromMilliseconds(50));
  EXPECT_EQ(0.5, slide_animation_->GetCurrentValue());

  // Reverse the animation and run it for half of the remaining duration.
  slide_animation_->Hide();
  RunAnimationFor(base::TimeDelta::FromMilliseconds(25));
  EXPECT_EQ(0.25, slide_animation_->GetCurrentValue());

  // Reverse the animation again and run it for half of the remaining duration.
  slide_animation_->Show();
  RunAnimationFor(base::TimeDelta::FromMillisecondsD(37.5));
  EXPECT_EQ(0.625, slide_animation_->GetCurrentValue());
}

// Tests that animations which are started partway and have a dampening factor
// of >1 progress properly.
TEST_F(SlideAnimationTest,
       AnimationWithPartialProgressAndNonDefaultDampeningFactor) {
  slide_animation_->SetTweenType(Tween::LINEAR);
  const double duration = 100;
  slide_animation_->SetDampeningValue(2.0);
  slide_animation_->SetSlideDuration(duration);
  slide_animation_->Show();
  // Advance the animation to halfway done.
  RunAnimationFor(base::TimeDelta::FromMilliseconds(duration / 2));
  EXPECT_EQ(0.5, slide_animation_->GetCurrentValue());

  // Reverse the animation and run it for half of the remaining duration.
  slide_animation_->Hide();
  float remaining_duration = 100 * pow(0.5, 2);
  RunAnimationFor(base::TimeDelta::FromMillisecondsD(remaining_duration / 2));
  // Half of the remaining progress is 0.25.
  EXPECT_FLOAT_EQ(0.25, slide_animation_->GetCurrentValue());

  // Reverse the animation again and run it for half of the remaining duration.
  slide_animation_->Show();
  remaining_duration = 100 * (1 - pow(0.25, 2));
  RunAnimationFor(base::TimeDelta::FromMillisecondsD(remaining_duration / 2));
  // Half of the remaining animation progress is 0.625.
  EXPECT_FLOAT_EQ(0.625, slide_animation_->GetCurrentValue());
}

}  // namespace gfx
