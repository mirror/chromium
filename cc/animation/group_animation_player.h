// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_GROUP_ANIMATION_PLAYER_H_
#define CC_ANIMATION_GROUP_ANIMATION_PLAYER_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "cc/animation/animation.h"
#include "cc/animation/animation_curve.h"
#include "cc/animation/animation_export.h"
#include "cc/animation/element_animations.h"
#include "cc/trees/element_id.h"

namespace cc {

// class AnimationDelegate;
// class AnimationEvents;
class AnimationHost;
class AnimationPlayer;
class AnimationTimeline;
// struct AnimationEvent;
// struct PropertyAnimationState;

class CC_ANIMATION_EXPORT GroupAnimationPlayer
    : public base::RefCounted<GroupAnimationPlayer> {
 public:
  static scoped_refptr<GroupAnimationPlayer> Create(int id);
  scoped_refptr<GroupAnimationPlayer> CreateImplInstance() const;

  int id() const { return id_; }

  // Parent AnimationHost. AnimationPlayer can be detached from
  // AnimationTimeline.
  AnimationHost* animation_host() { return animation_host_; }
  const AnimationHost* animation_host() const { return animation_host_; }
  void SetAnimationHost(AnimationHost* animation_host);

  // Parent AnimationTimeline.
  AnimationTimeline* animation_timeline() { return animation_timeline_; }
  const AnimationTimeline* animation_timeline() const {
    return animation_timeline_;
  }
  void SetAnimationTimeline(AnimationTimeline* timeline);

  bool needs_push_properties() const { return needs_push_properties_; }
  void SetNeedsPushProperties(AnimationPlayer&);

 private:
  friend class base::RefCounted<GroupAnimationPlayer>;

  explicit GroupAnimationPlayer(int id);
  ~GroupAnimationPlayer();

  void RegisterPlayer(AnimationPlayer&);
  void UnregisterPlayer(AnimationPlayer&);

  void BindElementAnimations(AnimationPlayer&);
  void UnbindElementAnimations(AnimationPlayer&);

  AnimationHost* animation_host_;
  AnimationTimeline* animation_timeline_;

  typedef base::ObserverList<AnimationPlayer> AnimationPlayers;
  AnimationPlayers animation_players_;

  int id_;
  bool needs_push_properties_;

  DISALLOW_COPY_AND_ASSIGN(GroupAnimationPlayer);
};

}  // namespace cc

#endif  // CC_ANIMATION_GROUP_ANIMATION_PLAYER_H_
