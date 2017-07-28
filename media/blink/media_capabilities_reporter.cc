// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/media_capabilities_reporter.h"

#include <cmath>
#include <limits>

#include "base/logging.h"
#include "base/macros.h"

namespace media {

// TODO(chcunningham): Find some authoritative list of framerates.
const int MediaCapabilitiesReporter::kFrameRateBuckets[] = {
    10, 20, 25, 30, 40, 50, 60, 70, 80, 90, 100, 120, 150, 200, 250, 300};

MediaCapabilitiesReporter::MediaCapabilitiesReporter(
    mojom::MediaCapabilitiesRecorderPtr recorder_ptr,
    const GetPipelineStatsCB& get_pipeline_stats_cb,
    const VideoDecoderConfig& video_config,
    std::unique_ptr<base::TickClock> tick_clock)
    : recorder_ptr_(std::move(recorder_ptr)),
      get_pipeline_stats_cb_(get_pipeline_stats_cb),
      video_config_(video_config),
      natural_size_(video_config.natural_size()),
      tick_clock_(std::move(tick_clock)),
      stats_cb_timer_(tick_clock_.get()) {}  // FIXME UMA

MediaCapabilitiesReporter::~MediaCapabilitiesReporter() {
  recorder_ptr_->FinalizeRecord();
}

void MediaCapabilitiesReporter::OnPlaying() {
  DVLOG(2) << __func__;

  if (is_playing_)
    return;
  is_playing_ = true;

  DCHECK(!stats_cb_timer_.IsRunning());

  if (ShouldBeReporting()) {
    RunStatsTimerAtIntervalMs(kRecordingIntervalMs);
  }
}

void MediaCapabilitiesReporter::OnPaused() {
  DVLOG(2) << __func__;

  if (!is_playing_)
    return;
  is_playing_ = false;

  // Stop timer until playing resumes.
  stats_cb_timer_.AbandonAndStop();
}

void MediaCapabilitiesReporter::OnNaturalSizeChanged(
    const gfx::Size& natural_size) {
  DVLOG(2) << __func__ << " " << natural_size.ToString();

  if (natural_size == natural_size_)
    return;

  natural_size_ = natural_size;

  // We haven't seen any pipeline stats for this size yet, so it may be that
  // the framerate will change too. Once framerate stabilized, UpdateStats()
  // will call the recorder_ptr_->StartNewRecord() with this latest size.
  ResetFramerateState();

  // Resetting the framerate state may allow us to begin reporting again. Avoid
  // calling if already running, as this will increase the delay for the next
  // UpdateStats() callback.
  if (ShouldBeReporting() && !stats_cb_timer_.IsRunning())
    RunStatsTimerAtIntervalMs(kRecordingIntervalMs);
}

void MediaCapabilitiesReporter::OnVideoConfigChanged(
    const VideoDecoderConfig& video_config) {
  DVLOG(2) << __func__ << " " << video_config.AsHumanReadableString();

  if (video_config.Matches(video_config_))
    return;

  video_config_ = video_config;
  natural_size_ = video_config.natural_size();

  // Force framerate detection for the new config. Once framerate stabilized,
  // UpdateStats() will call the recorder_ptr_->StartNewRecord() with this
  // latest config.
  ResetFramerateState();

  // Resetting the framerate state may allow us to begin reporting again. Also,
  // if the timer is already running we still want to reset the timer to give
  // the pipeline a chance to stabilize. Config changes may trigger little
  // hiccups while the decoder is reset.
  if (ShouldBeReporting())
    RunStatsTimerAtIntervalMs(kRecordingIntervalMs);
}

void MediaCapabilitiesReporter::OnHidden() {
  DVLOG(2) << __func__;

  if (is_backgrounded_)
    return;

  is_backgrounded_ = true;

  // Stop timer until no longer hidden.
  stats_cb_timer_.AbandonAndStop();
}

void MediaCapabilitiesReporter::OnShown() {
  DVLOG(2) << __func__;

  if (!is_backgrounded_)
    return;

  is_backgrounded_ = false;

  // Dropped frames are not reported during background rendering. Start a new
  // record to avoid reporting background stats.
  PipelineStatistics stats = get_pipeline_stats_cb_.Run();
  StartNewRecord(stats.video_frames_decoded, stats.video_frames_dropped);

  if (ShouldBeReporting())
    RunStatsTimerAtIntervalMs(kRecordingIntervalMs);
}

int MediaCapabilitiesReporter::GetFrameRateBucket(
    const PipelineStatistics& stats) const {
  if (stats.video_frame_duration_average.InSecondsF() == 0.0)
    return 0;

  int rounded_framerate =
      std::round(1 / stats.video_frame_duration_average.InSecondsF());

  // Find the closest match.
  // TODO(chcunningham): optimize with binary search/stl.
  int closest_framerate = std::numeric_limits<int>::lowest();
  for (int framerate_bucket : kFrameRateBuckets) {
    if (std::abs(framerate_bucket - rounded_framerate) >
        std::abs(closest_framerate - rounded_framerate)) {
      break;
    }

    closest_framerate = framerate_bucket;
  }

  return closest_framerate;
}

void MediaCapabilitiesReporter::RunStatsTimerAtIntervalMs(double milliseconds) {
  DVLOG(2) << __func__ << " " << milliseconds << " ms";
  DCHECK(ShouldBeReporting());

  // NOTE: Avoid optimizing with early returns  if the timer is already running
  // at |milliseconds|. Calling Start below resets the timer clock and some
  // callers (e.g. OnVideoConfigChanged) rely on that behavior behavior.
  stats_cb_timer_.Start(FROM_HERE,
                        base::TimeDelta::FromMillisecondsD(milliseconds), this,
                        &MediaCapabilitiesReporter::UpdateStats);
}

void MediaCapabilitiesReporter::StartNewRecord(uint32_t frames_decoded_offset,
                                               uint32_t frames_dropped_offset) {
  DVLOG(2) << __func__ << " "
           << " profile:" << video_config_.profile()
           << " fps:" << last_observed_fps_
           << " size:" << natural_size_.ToString();

  // New records decoded and dropped counts should start at zero.
  // These should never move backward.
  DCHECK_GE(frames_decoded_offset, frames_decoded_offset_);
  DCHECK_GE(frames_dropped_offset, frames_dropped_offset_);
  frames_decoded_offset_ = frames_decoded_offset;
  frames_dropped_offset_ = frames_dropped_offset;
  recorder_ptr_->StartNewRecord(video_config_.profile(), natural_size_,
                                last_observed_fps_);
}

void MediaCapabilitiesReporter::ResetFramerateState() {
  // Reinitialize all framerate state. The next UpdateStats() call will detect
  // the framerate.
  last_observed_fps_ = 0;
  num_stable_fps_samples_ = 0;
  num_unstable_fps_changes_ = 0;
  num_consecutive_tiny_fps_windows_ = 0;
  fps_stabilization_failed_ = false;
}

bool MediaCapabilitiesReporter::ShouldBeReporting() const {
  return is_playing_ && !is_backgrounded_ && !fps_stabilization_failed_;
}

bool MediaCapabilitiesReporter::AssessDecodeProgress(
    const PipelineStatistics& stats) {
  DCHECK_GE(stats.video_frames_decoded, last_frames_decoded_);
  DCHECK_GE(stats.video_frames_dropped, last_frames_dropped_);
  DCHECK_GE(stats.video_frames_decoded, stats.video_frames_dropped);

  // No-op if no additional frames decoded since last check.
  if (stats.video_frames_decoded == last_frames_decoded_) {
    num_consecutive_no_decode_progress_++;

    // Relax timer if its set to a short interval for framerate stabilization.
    if (stats_cb_timer_.GetCurrentDelay().InMilliseconds() <
        kRecordingIntervalMs) {
      DVLOG(2) << __func__ << " No decode progress; slowing the timer";
      RunStatsTimerAtIntervalMs(kRecordingIntervalMs);
    }
    return false;
  }

  num_consecutive_no_decode_progress_ = 0;
  last_frames_decoded_ = stats.video_frames_decoded;
  last_frames_dropped_ = stats.video_frames_dropped;

  return true;
}

bool MediaCapabilitiesReporter::AssessFramerateStability(
    const PipelineStatistics& stats) {
  int framerate = GetFrameRateBucket(stats);

  // When (re)initializing, the pipeline may momentarily return an average frame
  // duration of zero. Ignore it and wait for a real framerate.
  if (framerate == 0)
    return false;

  if (framerate != last_observed_fps_) {
    DVLOG(2) << __func__ << " fps changed: " << last_observed_fps_ << " -> "
             << framerate;
    last_observed_fps_ = framerate;
    num_stable_fps_samples_ = 1;
    num_unstable_fps_changes_++;

    if (num_unstable_fps_changes_ >= kMaxUnstableFpsChanges) {
      // Looks like VFR video. Wait for some property (e.g. decoder config) to
      // change before trying again.
      DVLOG(2) << __func__ << " Unable to stabilize FPS. Stopping timer.";
      fps_stabilization_failed_ = true;
      stats_cb_timer_.AbandonAndStop();
      return false;
    }

    // Increase the timer frequency to quickly stabilize framerate. 3x the
    // frame duration is used as this should be enough for a few more frames to
    // be decoded, while also being much faster (for typical framerates) than
    // the regular stats polling interval.
    RunStatsTimerAtIntervalMs(
        3 * stats.video_frame_duration_average.InMillisecondsF());
    return false;
  }

  // Framerate matched last observed!
  num_unstable_fps_changes_ = 0;
  num_stable_fps_samples_++;

  // Wait for steady framerate to begin recording stats.
  if (num_stable_fps_samples_ < kRequiredStableFpsSamples) {
    DVLOG(2) << __func__ << " fps held, awaiting stable ("
             << num_stable_fps_samples_ << ")";
    return false;
  } else if (num_stable_fps_samples_ == kRequiredStableFpsSamples) {
    // Update tick time for this latest stabilization event.
    base::TimeTicks previous_fps_stabilized_ticks = last_fps_stabilized_ticks_;
    last_fps_stabilized_ticks_ = tick_clock_->NowTicks();

    // Check if last stable FPS window was tiny.
    if (!previous_fps_stabilized_ticks.is_null() &&
        last_fps_stabilized_ticks_ - previous_fps_stabilized_ticks <
            base::TimeDelta::FromMilliseconds(kTinyFpsWindowMs)) {
      num_consecutive_tiny_fps_windows_++;
      DVLOG(2) << __func__ << " Last FPS window was 'tiny'. num_tiny:"
               << num_consecutive_tiny_fps_windows_;

      // Check for too many tiny FPS windows.
      if (num_consecutive_tiny_fps_windows_ >= kMaxTinyFpsWindows) {
        DVLOG(2) << __func__ << " Too many tiny fps windows. Stopping timer";
        fps_stabilization_failed_ = true;
        stats_cb_timer_.AbandonAndStop();
        return false;
      }
    } else {
      num_consecutive_tiny_fps_windows_ = 0;
    }

    // FPS is locked in. Start a new record, and set timer to reporting
    // interval.
    StartNewRecord(stats.video_frames_decoded, stats.video_frames_dropped);
    RunStatsTimerAtIntervalMs(kRecordingIntervalMs);
  }
  return true;
}

void MediaCapabilitiesReporter::UpdateStats() {
  DCHECK(ShouldBeReporting());

  PipelineStatistics stats = get_pipeline_stats_cb_.Run();
  DVLOG(2) << __func__ << " Raw stats -- dropped:" << stats.video_frames_dropped
           << "/" << stats.video_frames_decoded
           << " dur_avg:" << stats.video_frame_duration_average;

  if (!AssessDecodeProgress(stats))
    return;

  if (!AssessFramerateStability(stats))
    return;

  uint32_t frames_decoded = stats.video_frames_decoded - frames_decoded_offset_;
  uint32_t frames_dropped = stats.video_frames_dropped - frames_dropped_offset_;
  DCHECK_GE(frames_decoded, 0U);
  DCHECK_GE(frames_dropped, 0U);

  // Don't bother recording the first record immediately after stabilization.
  // Counts of zero don't add value.
  if (stats.video_frames_decoded == frames_decoded_offset_)
    return;

  DVLOG(2) << __func__ << " Recording -- dropped:" << frames_dropped << "/"
           << frames_decoded;
  recorder_ptr_->UpdateRecord(frames_decoded, frames_dropped);
}

}  // namespace media