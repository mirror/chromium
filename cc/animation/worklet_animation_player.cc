// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/animation/worklet_animation_player.h"

#include "base/memory/ptr_util.h"
#include "base/time/time.h"

namespace cc {

// A timing model that allows its output to be controlled directly ignoring the
// input time.
class ManagedTimingModel : public AnimationPlayer::TimingModel {
 public:
  ManagedTimingModel() = default;

  void set_local_time(base::TimeTicks time) { this->local_time_ = time; }

  base::TimeTicks GetValue(base::TimeTicks t) const override {
    return local_time_;
  }

  std::unique_ptr<TimingModel> Clone() const override {
    return base::WrapUnique<ManagedTimingModel>(new ManagedTimingModel(*this));
  }

 private:
  explicit ManagedTimingModel(const ManagedTimingModel& other)
      : local_time_(other.local_time_) {}

  base::TimeTicks local_time_;
};

WorkletAnimationPlayer::WorkletAnimationPlayer(int id)
    : AnimationPlayer(id, std::make_unique<ManagedTimingModel>()) {}

WorkletAnimationPlayer::~WorkletAnimationPlayer() {}

void WorkletAnimationPlayer::SetLocalTime(base::TimeTicks local_time) {
  DCHECK(timing_model_);
  static_cast<ManagedTimingModel*>(this->timing_model_.get())
      ->set_local_time(local_time);
}

}  // namespace cc
