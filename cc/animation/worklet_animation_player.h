// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_WORKLET_ANIMATION_PLAYER_H_
#define CC_WORKLET_ANIMATION_PLAYER_H_

#include "base/time/time.h"
#include "cc/animation/animation_export.h"
#include "cc/animation/animation_player.h"

namespace cc {

class ManagedTimingModel;

// A WorkletAnimationPlayer is an animations player that allows its animation
// timing to be controlled by an animator instance that is running in a
// AnimationWorkletGlobalScope.
class CC_ANIMATION_EXPORT WorkletAnimationPlayer final
    : public AnimationPlayer {
 public:
  WorkletAnimationPlayer(int id, const std::string& name);
  static scoped_refptr<WorkletAnimationPlayer> Create(int id,
                                                      const std::string& name);

  const std::string& name() { return name_; }
  void SetLocalTime(base::TimeDelta local_time);
  bool IsWorkletAnimationPlayer() const override;

 protected:
  void PushPropertiesToImplThread(
      AnimationPlayer* animation_player_impl) override;

 private:
  ~WorkletAnimationPlayer() override;

  ManagedTimingModel& timing() const;

  std::string name_;
};

}  // namespace cc

#endif
