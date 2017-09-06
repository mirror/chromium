// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TREES_IMAGE_ANIMATION_CONTROLLER_H_
#define CC_TREES_IMAGE_ANIMATION_CONTROLLER_H_

#include "base/cancelable_callback.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "cc/cc_export.h"
#include "cc/paint/discardable_image_map.h"
#include "cc/paint/image_id.h"
#include "cc/paint/paint_image.h"
#include "cc/paint/paint_image_generator.h"
#include "cc/tiles/tile_priority.h"

namespace cc {
class PaintImage;

class CC_EXPORT ImageAnimationController {
 public:
  // AnimationDrivers are clients interested in driving these animations. An
  // animation is ticked if there is at least one driver displaying it. Once
  // no drivers are registered for an image, or none of the registered drivers
  // want us to animate, the animation is no longer ticked.
  class CC_EXPORT AnimationDriver {
   public:
    virtual ~AnimationDriver() {}

    // Returns true if the image should be animated.
    virtual bool ShouldAnimate(PaintImage::Id paint_image_id) const = 0;
  };

  ImageAnimationController(base::SingleThreadTaskRunner* task_runner,
                           base::Closure invalidation_callback);
  ~ImageAnimationController();

  // Called to update the state for a PaintImage received in a new recording.
  void UpdateAnimatedImage(
      const DiscardableImageMap::AnimatedImageMetadata& data);

  // Registers/Unregisters an animation driver interested in animating this
  // image.
  // Note that the state for this image must have been populated to the
  // controller using UpdatePaintImage prior to registering any drivers.
  void RegisterAnimationDriver(PaintImage::Id paint_image_id,
                               AnimationDriver* driver);
  void UnregisterAnimationDriver(PaintImage::Id paint_image_id,
                                 AnimationDriver* driver);

  // Called to proceed the animations to the frame to be used on the sync tree.
  // This should be called only once for a sync tree and must be followed with
  // a call to DidActivateSyncTree when this tree is activated.
  // Returns the set of images that were animated and should be invalidated on
  // this sync tree.
  const PaintImageIdFlatSet& AnimateForSyncTree(base::TimeTicks now);

  // Called whenever the ShouldAnimate response for a driver could have changed.
  // This is called after every DrawProperties update and commit.
  void UpdateStateFromDrivers(base::TimeTicks now);

  // Called when the sync tree was activated and its associated state should be
  // pushed to the active tree.
  void DidActivate();

  // Returns the frame index to use for the given PaintImage and tree.
  size_t GetFrameIndexForImage(const PaintImage& paint_image,
                               WhichTree tree) const;

 private:
  class AnimationState {
   public:
    AnimationState();
    AnimationState(const AnimationState& other);
    ~AnimationState();

    bool ShouldAnimate() const;
    bool AdvanceFrame(base::TimeTicks now);
    void UpdateMetadata(const DiscardableImageMap::AnimatedImageMetadata& data);
    void PushPendingToActive();

    void AddDriver(AnimationDriver* driver);
    void RemoveDriver(AnimationDriver* driver);
    void UpdateStateFromDrivers();

    size_t pending_index() const { return pending_index_; }
    size_t active_index() const { return active_index_; }
    base::TimeTicks next_desired_frame_time() const {
      return next_desired_frame_time_;
    }

   private:
    size_t NextFrameIndex() const;

    PaintImage::Id paint_image_id_ = PaintImage::kNonLazyStableId;

    // The frame metadata received from the most updated recording with this
    // PaintImage.
    std::vector<FrameMetadata> frames_;

    // The number of animation loops requested for this image.
    int repetition_count_ = kAnimationNone;

    // The number of loops the animation has finished so far.
    int repetitions_completed_ = 0;

    // A set of drivers interested in animating this image.
    base::flat_set<AnimationDriver*> drivers_;

    // The index being used on the active tree, if a recording with this image
    // is still present.
    size_t active_index_ = PaintImage::kDefaultFrameIndex;

    // The index being displayed on the pending tree.
    size_t pending_index_ = PaintImage::kDefaultFrameIndex;

    // The time at which we would like to display the next frame. This can be in
    // the past, for instance, if we pause the animation from the image becoming
    // invisible.
    base::TimeTicks next_desired_frame_time_;

    // Set if there is at least one driver interested in animating this image,
    // cached from the last update.
    bool should_animate_from_drivers_ = false;

    // Set if the animation has been started.
    bool animation_started_ = false;

    // Whether the image is known to be completely loaded in the most recent
    // recording received.
    PaintImage::CompletionState completion_state_ =
        PaintImage::CompletionState::PARTIALLY_DONE;
  };

  class DelayedNotifier {
   public:
    DelayedNotifier(base::SingleThreadTaskRunner* task_runner,
                    base::Closure closure);
    ~DelayedNotifier();

    void Schedule(base::TimeTicks now, base::TimeTicks notification_time);
    void Cancel();
    void WillAnimate();

   private:
    void Notify();

    base::SingleThreadTaskRunner* task_runner_;
    base::Closure closure_;

    // Set if a notification is currently pending.
    base::Optional<base::TimeTicks> pending_notification_time_;

    // Set if the notification was dispatched and the resulting animation on the
    // next sync tree is pending.
    bool animation_pending_ = false;

    base::WeakPtrFactory<DelayedNotifier> weak_factory_;
  };

  // The AnimationState for images is persisted until they are cleared on
  // navigation. This is because while an image might not be painted anymore, if
  // it moves out of the interest rect for instance, the state retained is
  // necessary to resume the animation.
  // TODO(khushalsagar): Implement clearing of state on navigations.
  using AnimationStateMap = base::flat_map<PaintImage::Id, AnimationState>;
  AnimationStateMap animation_state_map_;

  // The set of currently active animations.
  PaintImageIdFlatSet active_animations_;

  // The set of images that were animated and invalidated on the last sync tree.
  PaintImageIdFlatSet images_animated_on_sync_tree_;

  DelayedNotifier notifier_;
};

}  // namespace cc

#endif  // CC_TREES_IMAGE_ANIMATION_CONTROLLER_H_
