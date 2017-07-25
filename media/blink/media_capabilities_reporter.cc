// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/media_capabilities_reporter.h"

#include <cmath>
#include <limits>

#include "base/logging.h"
#include "base/macros.h"

namespace {

// Slow interval for stats callbacks when no decode progress is being made (e.g.
// due to flaky network speeds).
const int kSlowTimerMs = 5000;

// Number of consecutive samples that must bucket to the same framerate in order
// for framerate to be considered "stable" enough to start recording stats.
const int kRequiredStableFpsSamples = 5;

// Limits the number attempts to detect stable framerate. When this limit is
// reached, we will give up until some stream property (e.g. decoder config)
// changes before trying again. We do not wish to record stats for variable
// framerate content.
const int kMaxUnstableFpsChanges = 10;

// Framerates must remain stable for a duration greater than this amount to
// avoid being classified as a "tiny fps window". See |kMaxTinyFpsWindows|.
const int kTinyFpsWindowMsec = 5000;

// Limits the number of consecutive "tiny fps windows", as defined by
// |kTinyFpsWindowMsec|. If this limit is met, we will give up until some stream
// property (e.g. decoder config) changes before trying again. We do not wish
// to record stats for variable framerate content.
const int kMaxTinyFpsWindows = 5;

}  // namespace

namespace media {

// TODO(chcunningham): Find some authoritative list of framerates.
const int MediaCapabilitiesReporter::kFrameRateBuckets[] = {
    10, 20, 25, 30, 40, 50, 60, 70, 80, 90, 100, 120, 150, 200, 250, 300};

MediaCapabilitiesReporter::MediaCapabilitiesReporter(
    mojom::MediaCapabilitiesRecorderPtr recorder_ptr,
    const GetPipelineStatsCB& get_pipeline_stats_cb,
    const VideoDecoderConfig& video_config)
    : recorder_ptr_(std::move(recorder_ptr)),
      get_pipeline_stats_cb_(get_pipeline_stats_cb),
      video_config_(video_config),
      natural_size_(video_config.natural_size()) {}  // FIXME UMA

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
    RunStatsTimerAtInterval(reporting_interval_);
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
    RunStatsTimerAtInterval(reporting_interval_);
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
    RunStatsTimerAtInterval(reporting_interval_);
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
    RunStatsTimerAtInterval(reporting_interval_);
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

void MediaCapabilitiesReporter::RunStatsTimerAtInterval(
    base::TimeDelta interval) {
  DVLOG(2) << __func__ << " " << interval.InSecondsF() << " sec";
  DCHECK(ShouldBeReporting());

  // This will reset the timer clock to begin waiting for |interval| to lapse.
  // NOTE: Avoid optimizing with early returns  if the timer is already running
  // at |interval|. Some callers (e.g. OnVideoConfigChanged) rely on the reset
  // behavior.
  stats_cb_timer_.Start(FROM_HERE, interval, this,
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

    // Relax timer if nothing is happening.
    if (num_consecutive_no_decode_progress_ == 10) {
      DVLOG(2) << __func__ << " No decode progress; slowing the timer";
      RunStatsTimerAtInterval(base::TimeDelta::FromMilliseconds(kSlowTimerMs));
    }
    return false;
  }

  if (num_consecutive_no_decode_progress_ > 0 &&
      stats_cb_timer_.GetCurrentDelay().InMilliseconds() == kSlowTimerMs) {
    // Just made progress for the first time in a while. Start using default
    // timer interval again.
    DVLOG(2) << __func__ << " New decode progress; using default timer";
    RunStatsTimerAtInterval(reporting_interval_);
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
    RunStatsTimerAtInterval(3 * stats.video_frame_duration_average);
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
    last_fps_stabilized_ticks_ = base::TimeTicks::Now();

    // Check if last stable FPS window was tiny.
    if (!previous_fps_stabilized_ticks.is_null() &&
        last_fps_stabilized_ticks_ - previous_fps_stabilized_ticks <
            base::TimeDelta::FromMilliseconds(kTinyFpsWindowMsec)) {
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
    RunStatsTimerAtInterval(reporting_interval_);
  }
  return true;
}

void MediaCapabilitiesReporter::UpdateStats() {
  DCHECK(ShouldBeReporting());

  PipelineStatistics stats = get_pipeline_stats_cb_.Run();

  if (!AssessDecodeProgress(stats))
    return;

  if (!AssessFramerateStability(stats))
    return;

  uint32_t frames_decoded = stats.video_frames_decoded - frames_decoded_offset_;
  uint32_t frames_dropped = stats.video_frames_dropped - frames_dropped_offset_;
  DVLOG(2) << __func__ << " Recording -- dropped:" << frames_dropped << "/"
           << frames_decoded;
  recorder_ptr_->UpdateRecord(frames_decoded, frames_dropped);
}

}  // namespace media