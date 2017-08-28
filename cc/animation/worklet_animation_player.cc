// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/worklet_animation_player.h"

#include "base/memory/ptr_util.h"
#include "base/time/time.h"

#include "base/logging.h"

namespace cc {

// A timing model that allows its output to be controlled directly ignoring the
// input time.
class ManagedTimingModel : public AnimationPlayer::TimingModel {
 public:
  ManagedTimingModel() = default;

  void set_local_time(base::TimeDelta local_time) {
    this->local_time_ = local_time;
  }
  base::TimeDelta local_time() { return this->local_time_; }

  base::TimeTicks GetValue(base::TimeTicks time,
                           const Animation& animation) const override {
    if (!animation.has_set_start_time()) {
      return base::TimeTicks();
    }

    // Animation player local time is equivalent to animation active time. So
    // we do the reverse of Animation::ConvertToActive here.
    // TODO(majidvp): This is fairly awkward. We should probably move
    // the TimingModel idea down to Animaiton and not use start and offset time
    return animation.start_time() + local_time_ - animation.time_offset();
  }

  std::unique_ptr<TimingModel> Clone() const override {
    return base::WrapUnique<ManagedTimingModel>(new ManagedTimingModel(*this));
  }

 private:
  explicit ManagedTimingModel(const ManagedTimingModel& other)
      : local_time_(other.local_time_) {}

  base::TimeDelta local_time_;
};

WorkletAnimationPlayer::WorkletAnimationPlayer(int id, const std::string& name)
    : AnimationPlayer(id, std::make_unique<ManagedTimingModel>()),
      name_(name) {}

WorkletAnimationPlayer::~WorkletAnimationPlayer() {}

scoped_refptr<WorkletAnimationPlayer> WorkletAnimationPlayer::Create(
    int id,
    const std::string& name) {
  return make_scoped_refptr(new WorkletAnimationPlayer(id, name));
}

ManagedTimingModel& WorkletAnimationPlayer::timing() const {
  return *(static_cast<ManagedTimingModel*>(this->timing_model_.get()));
}

void WorkletAnimationPlayer::SetLocalTime(base::TimeDelta local_time) {
  DCHECK(timing_model_);
  timing().set_local_time(local_time);
  SetNeedsPushProperties();
}

void WorkletAnimationPlayer::PushPropertiesToImplThread(
    AnimationPlayer* animation_player_impl) {
  AnimationPlayer::PushPropertiesToImplThread(animation_player_impl);
  static_cast<WorkletAnimationPlayer*>(animation_player_impl)
      ->SetLocalTime(timing().local_time());
}

bool WorkletAnimationPlayer::IsWorkletAnimationPlayer() const {
  return true;
}

}  // namespace cc
