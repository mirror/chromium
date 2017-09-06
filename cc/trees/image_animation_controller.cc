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
  animation_state.UpdateMetadata(data);
}

void ImageAnimationController::RegisterAnimationDriver(
    PaintImage::Id paint_image_id,
    AnimationDriver* driver) {
  auto it = animation_state_map_.find(paint_image_id);
  DCHECK(it != animation_state_map_.end());
  it->second.AddDriver(driver);
}

void ImageAnimationController::UnregisterAnimationDriver(
    PaintImage::Id paint_image_id,
    AnimationDriver* driver) {
  auto it = animation_state_map_.find(paint_image_id);
  DCHECK(it != animation_state_map_.end());
  it->second.RemoveDriver(driver);
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
    if (!state.ShouldAnimate())
      continue;

    // If we were able to advance this animation, invalidate it on the sync
    // tree.
    if (state.AdvanceFrame(now))
      images_animated_on_sync_tree_.insert(id);

    // Update the next invalidation time to the earliest time at which we need
    // a frame to animate an image.
    // Note its important to check ShouldAnimate() here again since advancing to
    // a new frame on the sync tree means we might not need to animate this
    // image any longer.
    if (state.ShouldAnimate()) {
      DCHECK_GT(state.next_desired_frame_time(), now);
      if (!next_invalidation_time.has_value()) {
        next_invalidation_time.emplace(state.next_desired_frame_time());
      } else {
        next_invalidation_time = std::min(state.next_desired_frame_time(),
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
    state.UpdateStateFromDrivers();

    // If we don't need to animate this image anymore, remove it from the list
    // of active animations.
    // Note that by not updating the |next_invalidation_time| from this image
    // here, we will cancel any pending invalidation scheduled for this image
    // when updating the |notifier_| at the end of this loop.
    if (!state.ShouldAnimate()) {
      active_animations_.erase(it.first);
      continue;
    }

    active_animations_.insert(it.first);
    if (!next_invalidation_time.has_value()) {
      next_invalidation_time.emplace(state.next_desired_frame_time());
    } else {
      next_invalidation_time = std::min(next_invalidation_time.value(),
                                        state.next_desired_frame_time());
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
    it->second.PushPendingToActive();
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
  return tree == WhichTree::PENDING_TREE ? it->second.pending_index()
                                         : it->second.active_index();
}

ImageAnimationController::AnimationState::AnimationState() = default;

ImageAnimationController::AnimationState::AnimationState(
    const AnimationState& other) = default;

ImageAnimationController::AnimationState::~AnimationState() = default;

bool ImageAnimationController::AnimationState::ShouldAnimate() const {
  // If we have no drivers for this image, no need to animate it.
  if (!should_animate_from_drivers_)
    return false;

  switch (repetition_count_) {
    case kAnimationLoopOnce:
      if (repetitions_completed_ >= 1)
        return false;
      break;
    case kAnimationNone:
      NOTREACHED() << "We shouldn't be tracking kAnimationNone images";
      break;
    case kAnimationLoopInfinite:
      break;
    default:
      if (repetition_count_ <= repetitions_completed_)
        return false;
  }

  // If we have all data for this image and the policy allows it, we can
  // continue animating it.
  if (completion_state_ == PaintImage::CompletionState::DONE)
    return true;

  // If we have not yet received all data for this image, we can not advance to
  // an incomplete frame.
  if (!frames_[NextFrameIndex()].complete)
    return false;

  // If we don't have all data for this image, we can not trust the frame count
  // and loop back to the first frame.
  size_t last_frame_index = frames_.size() - 1;
  if (pending_index_ == last_frame_index)
    return false;

  return true;
}

bool ImageAnimationController::AnimationState::AdvanceFrame(
    base::TimeTicks now) {
  DCHECK(ShouldAnimate());

  // Start the animation from the first frame, if not yet started.
  if (!animation_started_) {
    pending_index_ = 0u;
    next_desired_frame_time_ = now + frames_[0].duration;
    animation_started_ = true;
    return true;
  }

  // Don't advance the animation if its not time yet to move to the next frame.
  if (now < next_desired_frame_time_)
    return false;

  // If the animation is more than 5 min out of date, we don't bother catching
  // up and start again from the current frame.
  // Note that we don't need to invalidate this image since the active tree
  // is already displaying the current frame.
  if (now - next_desired_frame_time_ > kAnimationResyncCutoff) {
    DCHECK_EQ(pending_index_, active_index_);
    next_desired_frame_time_ = now + frames_[pending_index_].duration;
    return false;
  }

  // Keep catching up the animation until we reach the frame we should be
  // displaying now.
  while (next_desired_frame_time_ < now) {
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
    if (next_frame_index == 0u && repetition_count_ == 0 &&
        next_desired_frame_time_ < now) {
      pending_index_ = 0u;
      next_desired_frame_time_ = now + frames_[0].duration;
      break;
    }

    pending_index_ = next_frame_index;
    next_desired_frame_time_ += frames_[pending_index_].duration;

    size_t last_frame_index = frames_.size() - 1;
    if (pending_index_ == last_frame_index &&
        completion_state_ == PaintImage::CompletionState::DONE) {
      // If we are advancing to the last frame and we have all the data for this
      // image, we just finished a loop in the animation.
      repetition_count_++;
    }

    // We might have advanced to the last complete frame available, or reached
    // the desired repetition count, so check ShouldAnimate() again to see if we
    // should be proceeding further.
    if (!ShouldAnimate())
      break;
  }

  return true;
}

void ImageAnimationController::AnimationState::UpdateMetadata(
    const DiscardableImageMap::AnimatedImageMetadata& data) {
  paint_image_id_ = data.paint_image_id;

  DCHECK_NE(data.repetition_count, kAnimationNone);
  repetition_count_ = data.repetition_count;

  DCHECK(frames_.empty() || frames_.size() >= data.frames.size())
      << "Updated recordings can only append frames";
  frames_ = data.frames;

  DCHECK(completion_state_ != PaintImage::CompletionState::DONE ||
         data.completion_state == PaintImage::CompletionState::DONE)
      << "If the image was marked complete before, it can not be incomplete in "
         "a new update";
  completion_state_ = data.completion_state;

  // Update the repetition count in case we have displayed the last frame and
  // we now know the frame count to be accurate.
  size_t last_frame_index = frames_.size() - 1;
  if (completion_state_ == PaintImage::CompletionState::DONE &&
      pending_index_ == last_frame_index && repetition_count_ == 0)
    repetition_count_++;
}

void ImageAnimationController::AnimationState::PushPendingToActive() {
  active_index_ = pending_index_;
}

void ImageAnimationController::AnimationState::AddDriver(
    AnimationDriver* driver) {
  drivers_.insert(driver);
}

void ImageAnimationController::AnimationState::RemoveDriver(
    AnimationDriver* driver) {
  drivers_.erase(driver);
}

void ImageAnimationController::AnimationState::UpdateStateFromDrivers() {
  should_animate_from_drivers_ = false;
  for (auto* driver : drivers_)
    should_animate_from_drivers_ |= driver->ShouldAnimate(paint_image_id_);
}

size_t ImageAnimationController::AnimationState::NextFrameIndex() const {
  if (!animation_started_)
    return 0u;
  return (pending_index_ + 1) % frames_.size();
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
