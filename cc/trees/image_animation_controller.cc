// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/trees/image_animation_controller.h"

#include "base/bind.h"
#include "base/trace_event/trace_event.h"
#include "cc/paint/image_animation_count.h"

namespace cc {
namespace {

const base::TimeDelta kAnimationResyncCutoff = base::TimeDelta::FromMinutes(5);

}  //  namespace

ImageAnimationController::ImageAnimationController(
    base::SingleThreadTaskRunner* task_runner,
    base::Closure invalidation_callback)
    : notifier_(task_runner, invalidation_callback) {}

ImageAnimationController::~ImageAnimationController() = default;

void ImageAnimationController::UpdateAnimatedImage(
    const DiscardableImageMap::AnimatedImageMetadata& data) {
  AnimationState& animation_state = animation_state_map_[data.paint_image_id];
  animation_state.paint_image_id = data.paint_image_id;

  DCHECK_NE(data.repetition_count, kAnimationNone);
  animation_state.repetition_count = data.repetition_count;

  DCHECK(animation_state.frames.empty() ||
         animation_state.frames.size() >= data.frames.size())
      << "Updated recordings can only append frames";
  animation_state.frames = data.frames;

  DCHECK(animation_state.completion_state !=
             PaintImage::CompletionState::DONE ||
         data.completion_state == PaintImage::CompletionState::DONE)
      << "If the image was marked complete before, it can not be incomplete in "
         "a new update";
  animation_state.completion_state = data.completion_state;
}

void ImageAnimationController::RegisterAnimationDriver(
    PaintImage::Id paint_image_id,
    AnimationDriver* driver) {
  auto it = animation_state_map_.find(paint_image_id);
  DCHECK(it != animation_state_map_.end());
  it->second.drivers.insert(driver);
}

void ImageAnimationController::UnregisterAnimationDriver(
    PaintImage::Id paint_image_id,
    AnimationDriver* driver) {
  auto it = animation_state_map_.find(paint_image_id);
  DCHECK(it != animation_state_map_.end());
  it->second.drivers.erase(driver);
}

const PaintImageIdFlatSet& ImageAnimationController::AnimateForSyncTree(
    base::TimeTicks now) {
  TRACE_EVENT0("cc", "ImageAnimationController::AnimateImagesForSyncTree");
  DCHECK(images_animated_on_sync_tree_.empty());

  notifier_.WillAnimate();
  base::Optional<base::TimeTicks> next_invalidation_time;

  for (auto id : active_animations_) {
    auto it = animation_state_map_.find(id);
    DCHECK(it != animation_state_map_.end());
    AnimationState& state = it->second;

    // Is anyone still interested in animating this image?
    if (!state.should_animate)
      continue;

    if (now - state.next_desired_frame_time > kAnimationResyncCutoff) {
      // If the animation is more than 5 min out of date, don't catch up and
      // start again from the current frame.
      // Note that we don't need to invalidate this image since the active tree
      // is already displaying the current frame.
      DCHECK_EQ(state.pending_index, state.active_index);

      state.next_desired_frame_time =
          now + state.frames[state.pending_index].duration;
    } else if (state.AdvanceFrame(now)) {
      // If we were able to advance this animation, invalidate it on the sync
      // tree.
      images_animated_on_sync_tree_.insert(id);
    }

    // Update the next invalidation time to the earliest time at which we need
    // a frame to animate an image.
    // Note its important to check CanAnimate() here again since advancing to a
    // new frame on the sync tree means we might not need to animate this image
    // any longer.
    if (state.CanAnimate()) {
      DCHECK_GT(state.next_desired_frame_time, now);

      if (!next_invalidation_time.has_value()) {
        next_invalidation_time.emplace(state.next_desired_frame_time);
      } else {
        next_invalidation_time = std::min(state.next_desired_frame_time,
                                          next_invalidation_time.value());
      }
    }
  }

  if (next_invalidation_time.has_value())
    notifier_.Schedule(now, next_invalidation_time.value());
  else
    notifier_.Cancel();

  return images_animated_on_sync_tree_;
}

void ImageAnimationController::UpdateStateFromDrivers(base::TimeTicks now) {
  TRACE_EVENT0("cc", "UpdateStateFromAnimationDrivers");

  base::Optional<base::TimeTicks> next_invalidation_time;
  for (auto& it : animation_state_map_) {
    AnimationState& state = it.second;

    // Pull the latest value from the drivers.
    bool updated_should_animate = state.ShouldAnimate();
    if (state.should_animate == updated_should_animate)
      continue;
    state.should_animate = updated_should_animate;

    // If we don't need to animate this image anymore, remove it from the list
    // of active animations.
    // Note that by not updating the |next_invalidation_time| from this image
    // here, we will cancel any pending invalidation scheduled for this image
    // when updating the |notifier_| at the end of this loop.
    if (!state.should_animate) {
      active_animations_.erase(it.first);
      continue;
    }

    active_animations_.insert(it.first);
    if (!next_invalidation_time.has_value()) {
      next_invalidation_time.emplace(state.next_desired_frame_time);
    } else {
      next_invalidation_time = std::min(next_invalidation_time.value(),
                                        state.next_desired_frame_time);
    }
  }

  if (next_invalidation_time.has_value())
    notifier_.Schedule(now, next_invalidation_time.value());
  else
    notifier_.Cancel();
}

void ImageAnimationController::DidActivate() {
  TRACE_EVENT0("cc", "ImageAnimationController::WillActivate");

  for (auto id : images_animated_on_sync_tree_) {
    auto it = animation_state_map_.find(id);
    DCHECK(it != animation_state_map_.end());
    it->second.active_index = it->second.pending_index;
  }

  images_animated_on_sync_tree_.clear();
}

size_t ImageAnimationController::GetFrameIndexForImage(
    const PaintImage& paint_image,
    WhichTree tree) const {
  if (!paint_image.ShouldAnimate())
    return paint_image.frame_index();

  const auto& it = animation_state_map_.find(paint_image.stable_id());
  DCHECK(it != animation_state_map_.end());
  return tree == WhichTree::PENDING_TREE ? it->second.pending_index
                                         : it->second.active_index;
}

ImageAnimationController::AnimationState::AnimationState() = default;

ImageAnimationController::AnimationState::AnimationState(
    const AnimationState& other) = default;

ImageAnimationController::AnimationState::~AnimationState() = default;

bool ImageAnimationController::AnimationState::CanAnimate() const {
  switch (repetition_count) {
    case kAnimationLoopOnce:
      if (repetitions_completed >= 1)
        return false;
      break;
    case kAnimationNone:
      NOTREACHED() << "We shouldn't be tracking kAnimationNone images";
      break;
    case kAnimationLoopInfinite:
      break;
    default:
      if (repetition_count <= repetitions_completed)
        return false;
  }

  // If we have all data for this image and the policy allows it, we can
  // continue animating it.
  if (completion_state == PaintImage::CompletionState::DONE)
    return true;

  // If we have not yet received all data for this image, we can not advance to
  // an incomplete frame.
  if (!frames[NextFrameIndex()].complete)
    return false;

  // If we don't have all data for this image, we can not trust the frame count
  // and loop back to the first frame.
  size_t last_frame_index = frames.size() - 1;
  if (pending_index == last_frame_index)
    return false;

  return true;
}

bool ImageAnimationController::AnimationState::ShouldAnimate() const {
  if (!CanAnimate())
    return false;

  for (auto* driver : drivers) {
    if (driver->ShouldAnimate(paint_image_id))
      return true;
  }
  return false;
}

bool ImageAnimationController::AnimationState::AdvanceFrame(
    base::TimeTicks now) {
  DCHECK(ShouldAnimate());

  if (!animation_started) {
    pending_index = 0u;
    next_desired_frame_time = now + frames[0].duration;
    animation_started = true;
    return true;
  }

  // Don't advance the animation if its not time yet to move to the next frame.
  if (now < next_desired_frame_time)
    return false;

  // Keep catching the up the animation until we reach the frame we should be
  // displaying now.
  while (next_desired_frame_time < now) {
    size_t next_frame_index = NextFrameIndex();
    // The image may load more slowly than it's supposed to animate, so that by
    // the time we reach the end of the first repetition, we're well behind.
    // Start the animation from the first frame in this case, so that we don't
    // skip frames (or whole iterations) trying to "catch up".  This is a
    // tradeoff: It guarantees users see the whole animation the second time
    // through and don't miss any repetitions, and is closer to what other
    // browsers do; on the other hand, it makes animations "less accurate" for
    // pages that try to sync an image and some other resource (e.g. audio),
    // especially if users switch tabs (and thus stop drawing the animation,
    // which will pause it) during that initial loop, then switch back later.
    if (next_frame_index == 0u && repetition_count == 0 &&
        next_desired_frame_time < now) {
      pending_index = 0u;
      next_desired_frame_time = now + frames[0].duration;
      break;
    }

    size_t last_frame_index = frames.size() - 1;
    if (next_frame_index == last_frame_index &&
        completion_state == PaintImage::CompletionState::DONE) {
      // If we are advancing to the last frame and we have all the data for this
      // image, we just finished a loop in the animation.
      repetition_count++;
    }

    pending_index = next_frame_index;
    next_desired_frame_time += frames[pending_index].duration;
  }

  return true;
}

size_t ImageAnimationController::AnimationState::NextFrameIndex() const {
  if (!animation_started)
    return 0u;
  return (pending_index + 1) % frames.size();
}

ImageAnimationController::DelayedNotifier::DelayedNotifier(
    base::SingleThreadTaskRunner* task_runner,
    base::Closure closure)
    : task_runner_(task_runner),
      closure_(std::move(closure)),
      weak_factory_(this) {
  DCHECK(task_runner_->BelongsToCurrentThread());
}

ImageAnimationController::DelayedNotifier::~DelayedNotifier() {
  DCHECK(task_runner_->BelongsToCurrentThread());
}

void ImageAnimationController::DelayedNotifier::Schedule(
    base::TimeTicks now,
    base::TimeTicks notification_time) {
  // If an animation is already pending, don't schedule another invalidation.
  // We will schedule the next invalidation based on the latest animation state
  // during AnimateForSyncTree.
  if (animation_pending_)
    return;

  // The requested notification time can be in the past. For instance, if an
  // animation was paused because the image became invisible.
  if (notification_time < now)
    notification_time = now;

  if (pending_notification_time_.has_value() &&
      notification_time >= pending_notification_time_.value())
    return;

  // Cancel the pending notification since we need a notification sooner than
  // the previously scheduled run.
  Cancel();

  TRACE_EVENT2("cc", "ScheduleInvalidationForImageAnimation",
               "notification_time", notification_time, "now", now);
  pending_notification_time_.emplace(notification_time);
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::Bind(&DelayedNotifier::Notify, weak_factory_.GetWeakPtr()),
      notification_time - now);
}

void ImageAnimationController::DelayedNotifier::Cancel() {
  pending_notification_time_.reset();
  weak_factory_.InvalidateWeakPtrs();
}

void ImageAnimationController::DelayedNotifier::Notify() {
  pending_notification_time_.reset();
  animation_pending_ = true;
  closure_.Run();
}

void ImageAnimationController::DelayedNotifier::WillAnimate() {
  animation_pending_ = false;
}

}  // namespace cc
