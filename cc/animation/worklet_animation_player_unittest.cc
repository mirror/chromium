// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "cc/animation/worklet_animation_player.h"

#include "cc/test/animation_test_common.h"
#include "cc/test/animation_timelines_test_common.h"

namespace cc {
namespace {

class WorkletAnimationPlayerTest : public AnimationTimelinesTest {
 public:
  WorkletAnimationPlayerTest() {}
  ~WorkletAnimationPlayerTest() override {}

  int worklet_player_id_ = 11;
};

TEST_F(WorkletAnimationPlayerTest, LocalTimeIsUsedWithAnimations) {
  client_.RegisterElement(element_id_, ElementListType::ACTIVE);
  client_impl_.RegisterElement(element_id_, ElementListType::PENDING);
  client_impl_.RegisterElement(element_id_, ElementListType::ACTIVE);

  const float start_opacity = .7f;
  const float expected_opacity = .5f;
  const float end_opacity = .3f;
  const double duration = 1.;

  scoped_refptr<WorkletAnimationPlayer> worklet_player_ =
      WorkletAnimationPlayer::Create(worklet_player_id_);

  base::TimeTicks local_time =
      base::TimeTicks() + base::TimeDelta::FromSecondsD(duration / 2);
  worklet_player_->SetLocalTime(local_time);

  worklet_player_->AttachElement(element_id_);
  host_->AddAnimationTimeline(timeline_);
  timeline_->AttachPlayer(worklet_player_);

  AddOpacityTransitionToPlayer(worklet_player_.get(), duration, start_opacity,
                               end_opacity, false);

  EXPECT_TRUE(worklet_player_->needs_push_properties());

  host_->PushPropertiesTo(host_impl_);
  host_impl_->ActivateAnimations();

  base::TimeTicks time;
  time += base::TimeDelta::FromSecondsD(duration + 1);

  TickAnimationsTransferEvents(time, 1u);

  client_impl_.ExpectOpacityPropertyMutated(
      element_id_, ElementListType::ACTIVE, expected_opacity);
}

}  // namespace

}  // namespace cc
