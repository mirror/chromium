// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_WORKLET_ANIMATION_ANIMATION_PLAYER_H_
#define CC_WORKLET_ANIMATION_ANIMATION_PLAYER_H_

#include "base/time/time.h"
#include "cc/animation/animation_export.h"
#include "cc/animation/animation_player.h"

namespace cc {

// A WorkletAnimationPlayer is an animations player that allows its animation
// timing to be controlled by an animator instance that is running in a
// AnimationWorkletGlobalScope.
class CC_ANIMATION_EXPORT WorkletAnimationPlayer final
    : public AnimationPlayer {
 public:
  WorkletAnimationPlayer(int id);

  void SetLocalTime(base::TimeTicks local_time);

 private:
  ~WorkletAnimationPlayer() override;
};

}  // namespace cc

#endif
