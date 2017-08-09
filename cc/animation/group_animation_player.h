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

class AnimationDelegate;
class AnimationEvents;
class AnimationHost;
class AnimationPlayer;
class AnimationTimeline;
struct AnimationEvent;

class CC_ANIMATION_EXPORT GroupAnimationPlayer
    : public base::RefCounted<GroupAnimationPlayer> {
 public:
  using AnimationPlayers = std::vector<scoped_refptr<AnimationPlayer>>;

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
  // Detach player through timeline.
  // void DetachPlayer();
  void DetachElement();

  void set_animation_delegate(AnimationDelegate* delegate) {
    animation_delegate_ = delegate;
  }

  void Tick(base::TimeTicks monotonic_time);
  void UpdateState(bool start_ready_animations, AnimationEvents* events);

  // const PlayersList& ticking_players_for_testing() const;

  // Registers the given animation player as ticking. A ticking animation
  // player is one that has a running animation.
  void AddToTicking(scoped_refptr<AnimationPlayer> player);
  // void UpdateTickingState(UpdateTickingType type);
  // Unregisters the given animation player. When this happens, the
  // animation player will no longer be ticked.
  void RemoveFromTicking(scoped_refptr<AnimationPlayer> player);
  bool HasTickingPlayer() const { return !ticking_players_.empty(); }

  // AnimationDelegate routing.
  bool NotifyAnimationStarted(const AnimationEvent& event);
  bool NotifyAnimationFinished(const AnimationEvent& event);
  bool NotifyAnimationAborted(const AnimationEvent& event);
  void NotifyAnimationTakeover(const AnimationEvent& event);

  bool needs_to_start_animations() const { return needs_to_start_animations_; }

  bool needs_push_properties() const { return needs_push_properties_; }
  void SetNeedsPushProperties(AnimationPlayer&);
  void SetNeedsCommit();
  void PushPropertiesTo(GroupAnimationPlayer* group_player_impl);
  AnimationPlayer* GetAnimationPlayerById(int animation_player_id) const;

  void AttachPlayerToGroup(scoped_refptr<AnimationPlayer> animation_player);
  void DetachPlayerFromGroup(scoped_refptr<AnimationPlayer> animation_player);

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

  AnimationPlayers animation_players_;
  AnimationPlayers ticking_players_;

  AnimationDelegate* animation_delegate_;

  int id_;
  bool needs_push_properties_;
  base::TimeTicks last_tick_time_;

  // Only try to start animations when new animations are added or when the
  // previous attempt at starting animations failed to start all animations.
  bool needs_to_start_animations_;

  // This is used to ensure that we don't spam the animation host.
  // bool is_ticking_;

  std::unordered_map<int, scoped_refptr<AnimationPlayer>> id_to_player_map_;
  DISALLOW_COPY_AND_ASSIGN(GroupAnimationPlayer);
};

}  // namespace cc

#endif  // CC_ANIMATION_GROUP_ANIMATION_PLAYER_H_
