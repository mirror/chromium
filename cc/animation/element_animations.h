// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_ANIMATION_ELEMENT_ANIMATIONS_H_
#define CC_ANIMATION_ELEMENT_ANIMATIONS_H_

#include <bitset>
#include <memory>
#include <vector>

#include "base/containers/linked_list.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "cc/animation/animation.h"
#include "cc/animation/animation_curve.h"
#include "cc/animation/animation_events.h"
#include "cc/animation/target_property.h"
#include "cc/base/cc_export.h"
#include "ui/gfx/geometry/scroll_offset.h"
#include "ui/gfx/transform.h"

namespace gfx {
class BoxF;
}

namespace cc {

class AnimationDelegate;
class AnimationEvents;
class AnimationHost;
class AnimationPlayer;
class FilterOperations;
class KeyframeValueList;
enum class LayerTreeType;

// An ElementAnimations owns a list of all AnimationPlayers, attached to
// the layer.
// This is a CC counterpart for blink::ElementAnimations (in 1:1 relationship).
// No pointer to/from respective blink::ElementAnimations object for now.
class CC_EXPORT ElementAnimations : public base::RefCounted<ElementAnimations> {
 public:
  enum class ObserverType { ACTIVE, PENDING };

  static scoped_refptr<ElementAnimations> Create();

  int layer_id() const { return layer_id_; }
  void SetLayerId(int layer_id);

  // Parent AnimationHost.
  AnimationHost* animation_host() { return animation_host_; }
  const AnimationHost* animation_host() const { return animation_host_; }
  void SetAnimationHost(AnimationHost* host);

  void InitValueObservations();
  void ClearValueObservations();

  void LayerRegistered(int layer_id, LayerTreeType tree_type);
  void LayerUnregistered(int layer_id, LayerTreeType tree_type);

  void AddPlayer(AnimationPlayer* player);
  void RemovePlayer(AnimationPlayer* player);
  bool IsEmpty() const;

  typedef base::LinkedList<AnimationPlayer> PlayersList;
  typedef base::LinkNode<AnimationPlayer> PlayersListNode;
  const PlayersList& players_list() const { return *players_list_.get(); }

  // Ensures that the list of active animations on the main thread and the impl
  // thread are kept in sync. This function does not take ownership of the impl
  // thread ElementAnimations.
  void PushPropertiesTo(
      scoped_refptr<ElementAnimations> element_animations_impl);

  void AddAnimation(std::unique_ptr<Animation> animation);
  void PauseAnimation(int animation_id, base::TimeDelta time_offset);
  void RemoveAnimation(int animation_id);
  void AbortAnimation(int animation_id);
  void AbortAnimations(TargetProperty::Type target_property,
                       bool needs_completion = false);

  void Animate(base::TimeTicks monotonic_time);
  void AccumulatePropertyUpdates(base::TimeTicks monotonic_time,
                                 AnimationEvents* events);

  void UpdateState(bool start_ready_animations, AnimationEvents* events);

  // Make animations affect active observers if and only if they affect
  // pending observers. Any animations that no longer affect any observers
  // are deleted.
  void ActivateAnimations();

  // Returns the active animation animating the given property that is either
  // running, or is next to run, if such an animation exists.
  Animation* GetAnimation(TargetProperty::Type target_property) const;

  // Returns the active animation for the given unique animation id.
  Animation* GetAnimationById(int animation_id) const;

  // Returns true if there are any animations that have neither finished nor
  // aborted.
  bool HasActiveAnimation() const;

  // Returns true if there are any animations at all to process.
  bool has_any_animation() const { return !animations_.empty(); }

  // Returns true if there is an animation that is either currently animating
  // the given property or scheduled to animate this property in the future, and
  // that affects the given observer type.
  bool IsPotentiallyAnimatingProperty(TargetProperty::Type target_property,
                                      ObserverType observer_type) const;

  // Returns true if there is an animation that is currently animating the given
  // property and that affects the given observer type.
  bool IsCurrentlyAnimatingProperty(TargetProperty::Type target_property,
                                    ObserverType observer_type) const;

  void NotifyAnimationStarted(const AnimationEvent& event);
  void NotifyAnimationFinished(const AnimationEvent& event);
  void NotifyAnimationAborted(const AnimationEvent& event);
  void NotifyAnimationPropertyUpdate(const AnimationEvent& event);
  void NotifyAnimationTakeover(const AnimationEvent& event);

  bool needs_active_value_observations() const {
    return needs_active_value_observations_;
  }
  bool needs_pending_value_observations() const {
    return needs_pending_value_observations_;
  }

  void set_needs_active_value_observations(
      bool needs_active_value_observations) {
    needs_active_value_observations_ = needs_active_value_observations;
  }
  void set_needs_pending_value_observations(
      bool needs_pending_value_observations) {
    needs_pending_value_observations_ = needs_pending_value_observations;
  }

  bool HasFilterAnimationThatInflatesBounds() const;
  bool HasTransformAnimationThatInflatesBounds() const;
  bool HasAnimationThatInflatesBounds() const {
    return HasTransformAnimationThatInflatesBounds() ||
           HasFilterAnimationThatInflatesBounds();
  }

  bool FilterAnimationBoundsForBox(const gfx::BoxF& box,
                                   gfx::BoxF* bounds) const;
  bool TransformAnimationBoundsForBox(const gfx::BoxF& box,
                                      gfx::BoxF* bounds) const;

  bool HasAnimationThatAffectsScale() const;

  bool HasOnlyTranslationTransforms(ObserverType observer_type) const;

  bool AnimationsPreserveAxisAlignment() const;

  // Sets |start_scale| to the maximum of starting animation scale along any
  // dimension at any destination in active animations. Returns false if the
  // starting scale cannot be computed.
  bool AnimationStartScale(ObserverType observer_type,
                           float* start_scale) const;

  // Sets |max_scale| to the maximum scale along any dimension at any
  // destination in active animations. Returns false if the maximum scale cannot
  // be computed.
  bool MaximumTargetScale(ObserverType observer_type, float* max_scale) const;

  // When a scroll animation is removed on the main thread, its compositor
  // thread counterpart continues producing scroll deltas until activation.
  // These scroll deltas need to be cleared at activation, so that the active
  // layer's scroll offset matches the offset provided by the main thread
  // rather than a combination of this offset and scroll deltas produced by
  // the removed animation. This is to provide the illusion of synchronicity to
  // JS that simultaneously removes an animation and sets the scroll offset.
  bool scroll_offset_animation_was_interrupted() const {
    return scroll_offset_animation_was_interrupted_;
  }

  bool needs_to_start_animations_for_testing() {
    return needs_to_start_animations_;
  }

 private:
  friend class base::RefCounted<ElementAnimations>;

  ElementAnimations();
  ~ElementAnimations();

  // A set of target properties. TargetProperty must be 0-based enum.
  using TargetProperties =
      std::bitset<TargetProperty::LAST_TARGET_PROPERTY + 1>;

  void PushNewAnimationsToImplThread(
      ElementAnimations* element_animations_impl) const;
  void MarkAbortedAnimationsForDeletion(
      ElementAnimations* element_animations_impl) const;
  void RemoveAnimationsCompletedOnMainThread(
      ElementAnimations* element_animations_impl) const;
  void PushPropertiesToImplThread(ElementAnimations* element_animations_impl);

  void StartAnimations(base::TimeTicks monotonic_time);
  void PromoteStartedAnimations(base::TimeTicks monotonic_time,
                                AnimationEvents* events);
  void MarkFinishedAnimations(base::TimeTicks monotonic_time);
  void MarkAnimationsForDeletion(base::TimeTicks monotonic_time,
                                 AnimationEvents* events);
  void PurgeAnimationsMarkedForDeletion();

  void TickAnimations(base::TimeTicks monotonic_time);

  enum UpdateActivationType { NORMAL_ACTIVATION, FORCE_ACTIVATION };
  void UpdateActivation(UpdateActivationType type);

  void NotifyObserversOpacityAnimated(float opacity,
                                      bool notify_active_observers,
                                      bool notify_pending_observers);
  void NotifyObserversTransformAnimated(const gfx::Transform& transform,
                                        bool notify_active_observers,
                                        bool notify_pending_observers);
  void NotifyObserversFilterAnimated(const FilterOperations& filter,
                                     bool notify_active_observers,
                                     bool notify_pending_observers);
  void NotifyObserversScrollOffsetAnimated(
      const gfx::ScrollOffset& scroll_offset,
      bool notify_active_observers,
      bool notify_pending_observers);

  void NotifyObserversAnimationWaitingForDeletion();

  void NotifyObserversTransformIsPotentiallyAnimatingChanged(
      bool notify_active_observers,
      bool notify_pending_observers);

  void UpdatePotentiallyAnimatingTransform();

  void OnFilterAnimated(LayerTreeType tree_type,
                        const FilterOperations& filters);
  void OnOpacityAnimated(LayerTreeType tree_type, float opacity);
  void OnTransformAnimated(LayerTreeType tree_type,
                           const gfx::Transform& transform);
  void OnScrollOffsetAnimated(LayerTreeType tree_type,
                              const gfx::ScrollOffset& scroll_offset);
  void OnAnimationWaitingForDeletion();
  void OnTransformIsPotentiallyAnimatingChanged(LayerTreeType tree_type,
                                                bool is_animating);
  gfx::ScrollOffset ScrollOffsetForAnimation() const;

  void NotifyPlayersAnimationStarted(base::TimeTicks monotonic_time,
                                     TargetProperty::Type target_property,
                                     int group);
  void NotifyPlayersAnimationFinished(base::TimeTicks monotonic_time,
                                      TargetProperty::Type target_property,
                                      int group);
  void NotifyPlayersAnimationAborted(base::TimeTicks monotonic_time,
                                     TargetProperty::Type target_property,
                                     int group);
  void NotifyPlayersAnimationPropertyUpdate(const AnimationEvent& event);
  void NotifyPlayersAnimationTakeover(base::TimeTicks monotonic_time,
                                      TargetProperty::Type target_property,
                                      double animation_start_time,
                                      std::unique_ptr<AnimationCurve> curve);

  std::unique_ptr<PlayersList> players_list_;
  AnimationHost* animation_host_;
  int layer_id_;
  std::vector<std::unique_ptr<Animation>> animations_;

  // This is used to ensure that we don't spam the animation host.
  bool is_active_;

  base::TimeTicks last_tick_time_;

  bool needs_active_value_observations_;
  bool needs_pending_value_observations_;

  // Only try to start animations when new animations are added or when the
  // previous attempt at starting animations failed to start all animations.
  bool needs_to_start_animations_;

  bool scroll_offset_animation_was_interrupted_;

  bool potentially_animating_transform_for_active_observers_;
  bool potentially_animating_transform_for_pending_observers_;

  DISALLOW_COPY_AND_ASSIGN(ElementAnimations);
};

}  // namespace cc

#endif  // CC_ANIMATION_ELEMENT_ANIMATIONS_H_
