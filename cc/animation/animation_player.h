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
class AnimationTicker;
class AnimationTimeline;
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
// There is a 1:n relationship between AnimationPlayer and AnimationTicker which
// owns multiple cc side animations.
class CC_ANIMATION_EXPORT AnimationPlayer
    : public base::RefCounted<AnimationPlayer> {
 public:
  static scoped_refptr<AnimationPlayer> Create(int id);
  virtual scoped_refptr<AnimationPlayer> CreateImplInstance() const;

  int id() const { return id_; }
  virtual ElementId element_id() const;
  ElementId element_id_of_ticker(int ticker_id) const;
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

  virtual AnimationTicker* animation_ticker() const;

  bool has_element_animations() const;
  scoped_refptr<ElementAnimations> element_animations(int ticker_id) const;

  void set_animation_delegate(AnimationDelegate* delegate) {
    animation_delegate_ = delegate;
  }

  virtual void AttachElement(ElementId element_id);
  void AttachElementWithTicker(ElementId element_id, int ticker_id);
  virtual void DetachElement();

  void AddAnimationToTicker(std::unique_ptr<Animation> animation,
                            int ticker_id);
  void PauseAnimationOfTicker(int animation_id,
                              double time_offset,
                              int ticker_id);
  void RemoveAnimationFromTicker(int animation_id, int ticker_id);
  void AbortAnimationOfTicker(int animation_id, int ticker_id);
  void AbortAnimationsOfTicker(TargetProperty::Type target_property,
                               bool needs_completion,
                               int ticker_id);

  virtual void PushPropertiesTo(AnimationPlayer* player_impl);

  virtual void UpdateState(bool start_ready_animations,
                           AnimationEvents* events);
  void UpdateStateForTicker(bool start_ready_animations,
                            AnimationEvents* events,
                            int ticker_id);
  void Tick(base::TimeTicks monotonic_time);

  void AddToTicking();
  void AnimationRemovedFromTicking();

  // AnimationDelegate routing.
  void NotifyAnimationStarted(const AnimationEvent& event);
  void NotifyAnimationFinished(const AnimationEvent& event);
  void NotifyAnimationAborted(const AnimationEvent& event);
  void NotifyAnimationTakeover(const AnimationEvent& event);
  virtual size_t TickingAnimationsCount() const;
  size_t TickingAnimationsCountOfTicker(int ticker_id) const;

  void SetNeedsPushProperties();

  // Make animations affect active elements if and only if they affect
  // pending elements. Any animations that no longer affect any elements
  // are deleted.
  virtual void ActivateAnimations();
  void ActivateAnimationsOfTicker(int ticker_id);

  // Returns the animation animating the given property that is either
  // running, or is next to run, if such an animation exists.
  Animation* GetAnimationOfTicker(TargetProperty::Type target_property,
                                  int ticker_id) const;

  std::string ToString(int ticker_id) const;

  void SetNeedsCommit();

  virtual bool IsWorkletAnimationPlayer() const;
  void AddTicker(std::unique_ptr<AnimationTicker>);

  AnimationTicker* GetTickerById(int ticker_id) const;

 private:
  friend class base::RefCounted<AnimationPlayer>;

  void RegisterTicker(ElementId element_id, int ticker_id);
  virtual void UnregisterTicker();

  void PushAttachedTickersToImplThread(AnimationPlayer* player) const;
  void RemoveDetachedTickersFromImplThread(AnimationPlayer* player) const;
  void PushPropertiesToImplThread(AnimationPlayer* player);

 protected:
  explicit AnimationPlayer(int id);
  virtual ~AnimationPlayer();

  AnimationHost* animation_host_;
  AnimationTimeline* animation_timeline_;
  AnimationDelegate* animation_delegate_;

  int id_;

  using ElementToTickerIdMap =
      std::unordered_map<ElementId, std::unordered_set<int>, ElementIdHash>;
  using IdToTickerMap =
      std::unordered_map<int, std::unique_ptr<AnimationTicker>>;

  ElementToTickerIdMap element_to_ticker_id_map_;
  IdToTickerMap id_to_ticker_map_;

  DISALLOW_COPY_AND_ASSIGN(AnimationPlayer);
};

}  // namespace cc

#endif  // CC_ANIMATION_ANIMATION_PLAYER_H_
