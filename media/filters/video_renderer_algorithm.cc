// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/filters/video_renderer_algorithm.h"

#include <algorithm>
#include <limits>

namespace media {

// The number of frames to store for moving average calculations.  Value picked
// after experimenting with playback of various local media and YouTube clips.
const int kMovingAverageSamples = 25;

VideoRendererAlgorithm::ReadyFrame::ReadyFrame(
    const scoped_refptr<VideoFrame>& ready_frame)
    : frame(ready_frame),
      ideal_render_count(0),
      render_count(0),
      drop_count(0) {
}

VideoRendererAlgorithm::ReadyFrame::~ReadyFrame() {
}

bool VideoRendererAlgorithm::ReadyFrame::operator<(
    const ReadyFrame& other) const {
  return frame->timestamp() < other.frame->timestamp();
}

VideoRendererAlgorithm::VideoRendererAlgorithm(
    const TimeConverterCB& time_converter_cb)
    : cadence_estimator_(base::TimeDelta::FromSeconds(
          kMinimumAcceptableTimeBetweenGlitchesSecs)),
      time_converter_cb_(time_converter_cb),
      frame_duration_calculator_(kMovingAverageSamples),
      frame_dropping_disabled_(false) {
  DCHECK(!time_converter_cb_.is_null());
  Reset();
}

VideoRendererAlgorithm::~VideoRendererAlgorithm() {
}

scoped_refptr<VideoFrame> VideoRendererAlgorithm::Render(
    base::TimeTicks deadline_min,
    base::TimeTicks deadline_max,
    size_t* frames_dropped) {
  DCHECK_LT(deadline_min, deadline_max);

  if (frame_queue_.empty())
    return nullptr;

  if (frames_dropped)
    *frames_dropped = 0;

  // Once Render() is called |last_frame_index_| has meaning and should thus be
  // preserved even if better frames come in before it due to out of order
  // timestamps.
  have_rendered_frames_ = true;

  // Step 1: Update the current render interval for subroutines.
  render_interval_ = deadline_max - deadline_min;

  // Step 2: Figure out if any intervals have been skipped since the last call
  // to Render().  If so, we assume the last frame provided was rendered during
  // those intervals and adjust its render count appropriately.
  AccountForMissedIntervals(deadline_min, deadline_max);
  last_deadline_max_ = deadline_max;

  // Step 3: Update the wall clock timestamps and frame duration estimates for
  // all frames currently in the |frame_queue_|.
  if (!UpdateFrameStatistics()) {
    DVLOG(2) << "Failed to update frame statistics.";

    ReadyFrame& ready_frame = frame_queue_[last_frame_index_];
    DCHECK(ready_frame.frame);

    // If duration is unknown, we don't have enough frames to make a good guess
    // about which frame to use, so always choose the first.
    if (average_frame_duration_ == base::TimeDelta() &&
        !ready_frame.start_time.is_null()) {
      ++ready_frame.render_count;
    }

    return ready_frame.frame;
  }

  DCHECK_GT(average_frame_duration_, base::TimeDelta());

  base::TimeDelta selected_frame_drift;

  // Step 4: Attempt to find the best frame by cadence.
  int frame_to_render = FindBestFrameByCadence();
  if (frame_to_render >= 0) {
    selected_frame_drift =
        CalculateAbsoluteDriftForFrame(deadline_min, frame_to_render);
  }

  // Step 5: If no frame could be found by cadence or the selected frame exceeds
  // acceptable drift, try to find the best frame by coverage of the deadline.
  if (frame_to_render < 0 || selected_frame_drift > max_acceptable_drift_) {
    int second_best_by_coverage = -1;
    const int best_by_coverage = FindBestFrameByCoverage(
        deadline_min, deadline_max, &second_best_by_coverage);

    // If the frame was previously selected based on cadence, we're only here
    // because the drift is too large, so even if the cadence frame has the best
    // coverage, fallback to the second best by coverage if it has better drift.
    if (frame_to_render == best_by_coverage && second_best_by_coverage >= 0 &&
        CalculateAbsoluteDriftForFrame(deadline_min, second_best_by_coverage) <=
            selected_frame_drift) {
      frame_to_render = second_best_by_coverage;
    } else {
      frame_to_render = best_by_coverage;
    }

    if (frame_to_render >= 0) {
      selected_frame_drift =
          CalculateAbsoluteDriftForFrame(deadline_min, frame_to_render);
    }
  }

  // Step 6: If _still_ no frame could be found by coverage, try to choose the
  // least crappy option based on the drift from the deadline. If we're here the
  // selection is going to be bad because it means no suitable frame has any
  // coverage of the deadline interval.
  if (frame_to_render < 0 || selected_frame_drift > max_acceptable_drift_)
    frame_to_render = FindBestFrameByDrift(deadline_min, &selected_frame_drift);

  last_render_had_glitch_ = selected_frame_drift > max_acceptable_drift_;
  DVLOG_IF(2, last_render_had_glitch_)
      << "Frame drift is too far: " << selected_frame_drift.InMillisecondsF()
      << "ms";

  DCHECK_GE(frame_to_render, 0);

  // Drop some debugging information if a frame had poor cadence.
  if (cadence_estimator_.has_cadence()) {
    const ReadyFrame& last_frame_info = frame_queue_[last_frame_index_];
    if (static_cast<size_t>(frame_to_render) != last_frame_index_ &&
        last_frame_info.render_count < last_frame_info.ideal_render_count) {
      last_render_had_glitch_ = true;
      DVLOG(2) << "Under-rendered frame " << last_frame_info.frame->timestamp()
               << "; only " << last_frame_info.render_count
               << " times instead of " << last_frame_info.ideal_render_count;
    } else if (static_cast<size_t>(frame_to_render) == last_frame_index_ &&
               last_frame_info.render_count >=
                   last_frame_info.ideal_render_count) {
      DVLOG(2) << "Over-rendered frame " << last_frame_info.frame->timestamp()
               << "; rendered " << last_frame_info.render_count + 1
               << " times instead of " << last_frame_info.ideal_render_count;
      last_render_had_glitch_ = true;
    }
  }

  // Step 7: Drop frames which occur prior to the frame to be rendered. If any
  // frame has a zero render count it should be reported as dropped.
  if (frame_to_render > 0) {
    if (frames_dropped) {
      for (int i = 0; i < frame_to_render; ++i) {
        const ReadyFrame& frame = frame_queue_[i];
        if (frame.render_count != frame.drop_count)
          continue;

        // If frame dropping is disabled, ignore the results of the algorithm
        // and return the earliest unrendered frame.
        if (frame_dropping_disabled_) {
          frame_to_render = i;
          break;
        }

        DVLOG(2) << "Dropping unrendered (or always dropped) frame "
                 << frame.frame->timestamp()
                 << ", wall clock: " << frame.start_time.ToInternalValue()
                 << " (" << frame.render_count << ", " << frame.drop_count
                 << ")";
        ++(*frames_dropped);
        if (!cadence_estimator_.has_cadence() || frame.ideal_render_count)
          last_render_had_glitch_ = true;
      }
    }

    frame_queue_.erase(frame_queue_.begin(),
                       frame_queue_.begin() + frame_to_render);
  }

  if (last_render_had_glitch_) {
    DVLOG(2) << "Deadline: [" << deadline_min.ToInternalValue() << ", "
             << deadline_max.ToInternalValue()
             << "], Interval: " << render_interval_.InMicroseconds()
             << ", Duration: " << average_frame_duration_.InMicroseconds();
  }

  // Step 8: Congratulations, the frame selection gauntlet has been passed!
  last_frame_index_ = 0;
  ++frame_queue_.front().render_count;
  DCHECK(frame_queue_.front().frame);
  return frame_queue_.front().frame;
}

size_t VideoRendererAlgorithm::RemoveExpiredFrames(base::TimeTicks deadline) {
  // Update |last_deadline_max_| if it's no longer accurate; this should always
  // be done or EffectiveFramesQueued() may never expire the last frame.
  if (deadline > last_deadline_max_)
    last_deadline_max_ = deadline;

  if (!UpdateFrameStatistics() || frame_queue_.size() < 2)
    return 0;

  DCHECK_GT(average_frame_duration_, base::TimeDelta());

  // Finds and removes all frames which are too old to be used; I.e., the end of
  // their render interval is further than |max_acceptable_drift_| from the
  // given |deadline|.
  size_t frames_to_expire = 0;
  const base::TimeTicks minimum_start_time =
      deadline - max_acceptable_drift_ - average_frame_duration_;
  for (; frames_to_expire < frame_queue_.size() - 1; ++frames_to_expire) {
    if (frame_queue_[frames_to_expire].start_time >= minimum_start_time)
      break;
  }

  if (!frames_to_expire)
    return 0;

  frame_queue_.erase(frame_queue_.begin(),
                     frame_queue_.begin() + frames_to_expire);

  last_frame_index_ = last_frame_index_ > frames_to_expire
                          ? last_frame_index_ - frames_to_expire
                          : 0;
  return frames_to_expire;
}

void VideoRendererAlgorithm::OnLastFrameDropped() {
  // Since compositing is disconnected from the algorithm, the algorithm may be
  // Reset() in between ticks of the compositor, so discard notifications which
  // are invalid.
  //
  // If frames were expired by RemoveExpiredFrames() this count may be zero when
  // the OnLastFrameDropped() call comes in.
  if (!have_rendered_frames_ || frame_queue_.empty() ||
      !frame_queue_[last_frame_index_].render_count) {
    return;
  }

  ++frame_queue_[last_frame_index_].drop_count;
  DCHECK_LE(frame_queue_[last_frame_index_].drop_count,
            frame_queue_[last_frame_index_].render_count);
}

void VideoRendererAlgorithm::Reset() {
  last_frame_index_ = 0;
  have_rendered_frames_ = last_render_had_glitch_ = false;
  last_deadline_max_ = base::TimeTicks();
  average_frame_duration_ = render_interval_ = base::TimeDelta();
  frame_queue_.clear();
  cadence_estimator_.Reset();
  frame_duration_calculator_.Reset();

  // Default to ATSC IS/191 recommendations for maximum acceptable drift before
  // we have enough frames to base the maximum on frame duration.
  max_acceptable_drift_ = base::TimeDelta::FromMilliseconds(15);
}

size_t VideoRendererAlgorithm::EffectiveFramesQueued() const {
  if (frame_queue_.empty() || average_frame_duration_ == base::TimeDelta() ||
      last_deadline_max_.is_null()) {
    return frame_queue_.size();
  }

  // If we don't have cadence, subtract off any frames which are before
  // the last rendered frame or are past their expected rendering time.
  if (!cadence_estimator_.has_cadence()) {
    size_t expired_frames = last_frame_index_;
    DCHECK_LT(last_frame_index_, frame_queue_.size());
    for (; expired_frames < frame_queue_.size(); ++expired_frames) {
      const ReadyFrame& frame = frame_queue_[expired_frames];
      if (frame.end_time.is_null() || frame.end_time > last_deadline_max_)
        break;
    }
    return frame_queue_.size() - expired_frames;
  }

  // Find the first usable frame to start counting from.
  const int start_index = FindBestFrameByCadenceInternal(nullptr);
  if (start_index < 0)
    return 0;

  const base::TimeTicks minimum_start_time =
      last_deadline_max_ - max_acceptable_drift_;
  size_t renderable_frame_count = 0;
  for (size_t i = start_index; i < frame_queue_.size(); ++i) {
    const ReadyFrame& frame = frame_queue_[i];
    if (frame.render_count < frame.ideal_render_count &&
        (frame.end_time.is_null() || frame.end_time > minimum_start_time)) {
      ++renderable_frame_count;
    }
  }

  return renderable_frame_count;
}

void VideoRendererAlgorithm::EnqueueFrame(
    const scoped_refptr<VideoFrame>& frame) {
  DCHECK(frame);
  DCHECK(!frame->end_of_stream());

  ReadyFrame ready_frame(frame);
  auto it = frame_queue_.empty() ? frame_queue_.end()
                                 : std::lower_bound(frame_queue_.begin(),
                                                    frame_queue_.end(), frame);
  DCHECK_GE(it - frame_queue_.begin(), 0);

  // If a frame was inserted before the first frame, update the index. On the
  // next call to Render() it will be dropped.
  if (static_cast<size_t>(it - frame_queue_.begin()) <= last_frame_index_ &&
      have_rendered_frames_) {
    ++last_frame_index_;
  }

  // The vast majority of cases should always append to the back, but in rare
  // circumstance we get out of order timestamps, http://crbug.com/386551.
  it = frame_queue_.insert(it, ready_frame);

  // Project the current cadence calculations to include the new frame.  These
  // may not be accurate until the next Render() call.  These updates are done
  // to ensure EffectiveFramesQueued() returns a semi-reliable result.
  if (cadence_estimator_.has_cadence())
    UpdateCadenceForFrames();

#ifndef NDEBUG
  // Verify sorted order in debug mode.
  for (size_t i = 0; i < frame_queue_.size() - 1; ++i) {
    DCHECK(frame_queue_[i].frame->timestamp() <=
           frame_queue_[i + 1].frame->timestamp());
  }
#endif
}

void VideoRendererAlgorithm::AccountForMissedIntervals(
    base::TimeTicks deadline_min,
    base::TimeTicks deadline_max) {
  if (last_deadline_max_.is_null() || deadline_min <= last_deadline_max_ ||
      !have_rendered_frames_) {
    return;
  }

  DCHECK_GT(render_interval_, base::TimeDelta());
  const int64 render_cycle_count =
      (deadline_min - last_deadline_max_) / render_interval_;

  // In the ideal case this value will be zero.
  if (!render_cycle_count)
    return;

  DVLOG(2) << "Missed " << render_cycle_count << " Render() intervals.";

  // Only update render count if the frame was rendered at all; it may not have
  // been if the frame is at the head because we haven't rendered anything yet
  // or because previous frames were removed via RemoveExpiredFrames().
  ReadyFrame& ready_frame = frame_queue_[last_frame_index_];
  if (!ready_frame.render_count)
    return;

  // If the frame was never really rendered since it was dropped each attempt,
  // we need to increase the drop count as well to match the new render count.
  // Otherwise we won't properly count the frame as dropped when it's discarded.
  // We always update the render count so FindBestFrameByCadenceInternal() can
  // properly account for potentially over-rendered frames.
  if (ready_frame.render_count == ready_frame.drop_count)
    ready_frame.drop_count += render_cycle_count;
  ready_frame.render_count += render_cycle_count;
}

bool VideoRendererAlgorithm::UpdateFrameStatistics() {
  // Figure out all current ready frame times at once so we minimize the drift
  // relative to real time as the code below executes.
  for (size_t i = 0; i < frame_queue_.size(); ++i) {
    ReadyFrame& frame = frame_queue_[i];
    const bool new_frame = frame.start_time.is_null();
    frame.start_time = time_converter_cb_.Run(frame.frame->timestamp());

    // If time stops or never started, exit immediately.
    if (frame.start_time.is_null()) {
      frame.end_time = base::TimeTicks();
      return false;
    }

    // TODO(dalecurtis): An unlucky tick of a playback rate change could cause
    // this to skew so much that time goes backwards between calls.  Fix this by
    // either converting all timestamps at once or with some retry logic.
    if (i > 0) {
      frame_queue_[i - 1].end_time = frame.start_time;
      const base::TimeDelta delta =
          frame.start_time - frame_queue_[i - 1].start_time;
      CHECK_GT(delta, base::TimeDelta());
      if (new_frame)
        frame_duration_calculator_.AddSample(delta);
    }
  }

  if (!frame_duration_calculator_.count())
    return false;

  // Compute |average_frame_duration_|, a moving average of the last few frames;
  // see kMovingAverageSamples for the exact number.
  average_frame_duration_ = frame_duration_calculator_.Average();

  // Update the frame end time for the last frame based on the average.
  frame_queue_.back().end_time =
      frame_queue_.back().start_time + average_frame_duration_;

  // ITU-R BR.265 recommends a maximum acceptable drift of +/- half of the frame
  // duration; there are other asymmetric, more lenient measures, that we're
  // forgoing in favor of simplicity.
  //
  // We'll always allow at least 8.33ms of drift since literature suggests it's
  // well below the floor of detection.
  max_acceptable_drift_ = std::max(average_frame_duration_ / 2,
                                   base::TimeDelta::FromSecondsD(1.0 / 120));

  // If we were called via RemoveExpiredFrames() and Render() was never called,
  // we may not have a render interval yet.
  if (render_interval_ == base::TimeDelta())
    return true;

  const bool cadence_changed = cadence_estimator_.UpdateCadenceEstimate(
      render_interval_, average_frame_duration_, max_acceptable_drift_);

  // No need to update cadence if there's been no change; cadence will be set
  // as frames are added to the queue.
  if (!cadence_changed)
    return true;

  UpdateCadenceForFrames();

  // Thus far there appears to be no need for special 3:2 considerations, the
  // smoothness scores seem to naturally fit that pattern based on maximizing
  // frame coverage.
  return true;
}

void VideoRendererAlgorithm::UpdateCadenceForFrames() {
  for (size_t i = last_frame_index_; i < frame_queue_.size(); ++i) {
    // It's always okay to adjust the ideal render count, since the cadence
    // selection method will still count its current render count towards
    // cadence selection.
    frame_queue_[i].ideal_render_count =
        cadence_estimator_.has_cadence()
            ? cadence_estimator_.GetCadenceForFrame(i - last_frame_index_)
            : 0;
  }
}

int VideoRendererAlgorithm::FindBestFrameByCadence() {
  DCHECK(!frame_queue_.empty());
  if (!cadence_estimator_.has_cadence())
    return -1;

  int new_ideal_render_count = 0;
  const int best_frame =
      FindBestFrameByCadenceInternal(&new_ideal_render_count);
  if (best_frame < 0)
    return -1;

  DCHECK_GT(new_ideal_render_count, 0);
  frame_queue_[best_frame].ideal_render_count = new_ideal_render_count;
  return best_frame;
}

int VideoRendererAlgorithm::FindBestFrameByCadenceInternal(
    int* adjusted_ideal_render_count) const {
  DCHECK(!frame_queue_.empty());
  DCHECK(cadence_estimator_.has_cadence());
  const ReadyFrame& current_frame = frame_queue_[last_frame_index_];

  // If the current frame is below cadence, we should prefer it.
  if (current_frame.render_count < current_frame.ideal_render_count) {
    if (adjusted_ideal_render_count)
      *adjusted_ideal_render_count = current_frame.ideal_render_count;
    return last_frame_index_;
  }

  // For over-rendered frames we need to ensure we skip frames and subtract
  // each skipped frame's ideal cadence from the over-render count until we
  // find a frame which still has a positive ideal render count.
  int render_count_overage = std::max(
      0, current_frame.render_count - current_frame.ideal_render_count);

  // If the current frame is on cadence or over cadence, find the next frame
  // with a positive ideal render count.
  for (size_t i = last_frame_index_ + 1; i < frame_queue_.size(); ++i) {
    const ReadyFrame& frame = frame_queue_[i];
    if (frame.ideal_render_count > render_count_overage) {
      if (adjusted_ideal_render_count) {
        *adjusted_ideal_render_count =
            frame.ideal_render_count - render_count_overage;
      }
      return i;
    } else {
      // The ideal render count should always be zero or smaller than the
      // over-render count.
      render_count_overage -= frame.ideal_render_count;
      DCHECK_GE(render_count_overage, 0);
    }
  }

  // We don't have enough frames to find a better once by cadence.
  return -1;
}

int VideoRendererAlgorithm::FindBestFrameByCoverage(
    base::TimeTicks deadline_min,
    base::TimeTicks deadline_max,
    int* second_best) const {
  DCHECK(!frame_queue_.empty());

  // Find the frame which covers the most of the interval [deadline_min,
  // deadline_max]. Frames outside of the interval are considered to have no
  // coverage, while those which completely overlap the interval have complete
  // coverage.
  int best_frame_by_coverage = -1;
  base::TimeDelta best_coverage;
  std::vector<base::TimeDelta> coverage(frame_queue_.size(), base::TimeDelta());
  for (size_t i = last_frame_index_; i < frame_queue_.size(); ++i) {
    const ReadyFrame& frame = frame_queue_[i];

    // Frames which start after the deadline interval have zero coverage.
    if (frame.start_time > deadline_max)
      break;

    // Clamp frame end times to a maximum of |deadline_max|.
    const base::TimeTicks end_time = std::min(deadline_max, frame.end_time);

    // Frames entirely before the deadline interval have zero coverage.
    if (end_time < deadline_min)
      continue;

    // If we're here, the current frame overlaps the deadline in some way; so
    // compute the duration of the interval which is covered.
    const base::TimeDelta duration =
        end_time - std::max(deadline_min, frame.start_time);

    coverage[i] = duration;
    if (coverage[i] > best_coverage) {
      best_frame_by_coverage = i;
      best_coverage = coverage[i];
    }
  }

  // Find the second best frame by coverage; done by zeroing the coverage for
  // the previous best and recomputing the maximum.
  *second_best = -1;
  if (best_frame_by_coverage >= 0) {
    coverage[best_frame_by_coverage] = base::TimeDelta();
    auto it = std::max_element(coverage.begin(), coverage.end());
    if (*it > base::TimeDelta())
      *second_best = it - coverage.begin();
  }

  // If two frames have coverage within half a millisecond, prefer the earliest
  // frame as having the best coverage.  Value chosen via experimentation to
  // ensure proper coverage calculation for 24fps in 60Hz where +/- 100us of
  // jitter is present within the |render_interval_|. At 60Hz this works out to
  // an allowed jitter of 3%.
  const base::TimeDelta kAllowableJitter =
      base::TimeDelta::FromMicroseconds(500);
  if (*second_best >= 0 && best_frame_by_coverage > *second_best &&
      (best_coverage - coverage[*second_best]).magnitude() <=
          kAllowableJitter) {
    std::swap(best_frame_by_coverage, *second_best);
  }

  // TODO(dalecurtis): We may want to make a better decision about what to do
  // when multiple frames have equivalent coverage over an interval.  Jitter in
  // the render interval may result in irregular frame selection which may be
  // visible to a viewer.
  //
  // 23.974fps and 24fps in 60Hz are the most common susceptible rates, so
  // extensive tests have been added to ensure these cases work properly.

  return best_frame_by_coverage;
}

int VideoRendererAlgorithm::FindBestFrameByDrift(
    base::TimeTicks deadline_min,
    base::TimeDelta* selected_frame_drift) const {
  DCHECK(!frame_queue_.empty());

  int best_frame_by_drift = -1;
  *selected_frame_drift = base::TimeDelta::Max();

  for (size_t i = last_frame_index_; i < frame_queue_.size(); ++i) {
    const base::TimeDelta drift =
        CalculateAbsoluteDriftForFrame(deadline_min, i);
    // We use <= here to prefer the latest frame with minimum drift.
    if (drift <= *selected_frame_drift) {
      *selected_frame_drift = drift;
      best_frame_by_drift = i;
    }
  }

  return best_frame_by_drift;
}

base::TimeDelta VideoRendererAlgorithm::CalculateAbsoluteDriftForFrame(
    base::TimeTicks deadline_min,
    int frame_index) const {
  const ReadyFrame& frame = frame_queue_[frame_index];
  // If the frame lies before the deadline, compute the delta against the end
  // of the frame's duration.
  if (frame.end_time < deadline_min)
    return deadline_min - frame.end_time;

  // If the frame lies after the deadline, compute the delta against the frame's
  // start time.
  if (frame.start_time > deadline_min)
    return frame.start_time - deadline_min;

  // Drift is zero for frames which overlap the deadline interval.
  DCHECK_GE(deadline_min, frame.start_time);
  DCHECK_GE(frame.end_time, deadline_min);
  return base::TimeDelta();
}

}  // namespace media
