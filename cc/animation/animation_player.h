// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_ANIMATION_PLAYER_H_
#define CC_ANIMATION_ANIMATION_PLAYER_H_

#include <vector>

#include <memory>
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
class AnimationTimeline;
class AnimationTicker;
struct AnimationEvent;

// An AnimationPlayer manages grouped sets of animations (each set of which are
// stored in an AnimationTicker), and handles the interaction with the
// AnimationHost and AnimationTimeline.
//
// This class is a CC counterpart for blink::Animation, currently in a 1:1
// relationship. Currently the blink logic is responsible for handling of
// conflicting same-property animations.
//
// Each cc AnimationPlayer has a copy on the impl thread, and will take care of
// synchronizing properties to/from the impl thread when requested.
//
// NOTE(smcgruer): As of 2017/09/06 there is a 1:1 relationship between
// AnimationPlayer and the AnimationTicker. This is intended to become a 1:N
// relationship to allow for grouped animations.
class CC_ANIMATION_EXPORT AnimationPlayer
    : public base::RefCounted<AnimationPlayer> {
 public:
  static scoped_refptr<AnimationPlayer> Create(int id);
  virtual scoped_refptr<AnimationPlayer> CreateImplInstance() const;

  int id() const { return id_; }
  virtual ElementId element_id() const;
  bool IsElementAttached(ElementId id) const;

  // Parent AnimationHost. AnimationPlayer can be detached from
  // AnimationTimeline.
  AnimationHost* animation_host() { return animation_host_; }
  const AnimationHost* animation_host() const { return animation_host_; }
  void SetAnimationHost(AnimationHost* animation_host);
  bool has_animation_host() const { return !!animation_host_; }

  // Parent AnimationTimeline.
  AnimationTimeline* animation_timeline() { return animation_timeline_; }
  const AnimationTimeline* animation_timeline() const {
    return animation_timeline_;
  }
  virtual void SetAnimationTimeline(AnimationTimeline* timeline);

  AnimationTicker* animation_ticker() const;
  std::vector<AnimationTicker*> animation_tickers(ElementId element_id);

  //// TODO(smcgruer): Only used by a ui/ unittest: remove.
  // bool has_any_animation() const;

  // TODO(yigu): The return type is required only in tests. Boolean is used in
  // other cases. Need to support group effects. Could be removed for non-single
  // case.
  virtual scoped_refptr<ElementAnimations> element_animations() const;

  void set_animation_delegate(AnimationDelegate* delegate) {
    animation_delegate_ = delegate;
  }

  virtual void AttachElement(ElementId element_id);
  virtual void DetachElement();

  virtual void AddAnimation(std::unique_ptr<Animation> animation);
  virtual void PauseAnimation(int animation_id, double time_offset);
  virtual void RemoveAnimation(int animation_id);
  virtual void AbortAnimation(int animation_id);
  virtual void AbortAnimations(TargetProperty::Type target_property,
                               bool needs_completion);

  virtual void PushPropertiesTo(AnimationPlayer* player_impl);

  virtual void UpdateState(bool start_ready_animations,
                           AnimationEvents* events);
  virtual void Tick(base::TimeTicks monotonic_time);

  void AddToTicking();
  void AnimationRemovedFromTicking();

  // AnimationDelegate routing.
  void NotifyAnimationStarted(const AnimationEvent& event);
  void NotifyAnimationFinished(const AnimationEvent& event);
  void NotifyAnimationAborted(const AnimationEvent& event);
  void NotifyAnimationTakeover(const AnimationEvent& event);
  bool NotifyAnimationFinishedForTesting(TargetProperty::Type target_property,
                                         int group_id);
  size_t TickingAnimationsCount() const;

  void SetNeedsPushProperties();

  // Make animations affect active elements if and only if they affect
  // pending elements. Any animations that no longer affect any elements
  // are deleted.
  virtual void ActivateAnimations();

  // Returns the animation animating the given property that is either
  // running, or is next to run, if such an animation exists.
  virtual Animation* GetAnimation(TargetProperty::Type target_property) const;

  virtual std::string ToString() const;

  void SetNeedsCommit();

  virtual bool IsWorkletAnimationPlayer() const;
  void AddTicker(std::unique_ptr<AnimationTicker>);

  void DebugElementToTickerMap();

 private:
  friend class base::RefCounted<AnimationPlayer>;

  // void RegisterPlayer(ElementId element_id);
  // void UnregisterPlayer();
  void FakeMap();

 protected:
  explicit AnimationPlayer(int id);
  virtual ~AnimationPlayer();

  AnimationHost* animation_host_;
  AnimationTimeline* animation_timeline_;
  AnimationDelegate* animation_delegate_;

  int id_;

  AnimationTicker* GetUniqueTickerForSingleAnimationPlayer() const;
  std::unique_ptr<AnimationTicker> animation_ticker_;
  using ElementToTickerMap =
      std::unordered_map<ElementId,
                         std::unordered_set<std::unique_ptr<AnimationTicker>>,
                         ElementIdHash>;
  ElementToTickerMap element_to_ticker_map_;

  DISALLOW_COPY_AND_ASSIGN(AnimationPlayer);
};

}  // namespace cc

#endif  // CC_ANIMATION_ANIMATION_PLAYER_H_
