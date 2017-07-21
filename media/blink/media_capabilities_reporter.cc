// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/media_capabilities_reporter.h"

#include <cmath>
#include <limits>

#include "base/logging.h"
#include "base/macros.h"

namespace {

// Initial delay of |stats_cb_timer_|. Hopefully to be big enough for some
// initial pipeline statistics to be populated.
const int kStartingStatsDelayMs = 500;

// Slow interval for stats callbacks when no decode progress is being made (e.g.
// due to flaky network speeds).
const int kSlowTimerMs = 5000;

// Number of consecutive samples that must bucket to the same framerate in order
// for framerate to be considered "stable" enough to start recording stats.
const int kRequiredStableFpsSamples = 5;

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
      natural_size_(video_config.natural_size()) {}

MediaCapabilitiesReporter::~MediaCapabilitiesReporter() {
  recorder_ptr_->FinalizeRecord();
}

void MediaCapabilitiesReporter::OnPlaying() {
  DVLOG(2) << __func__;

  if (stats_cb_timer_.IsRunning())
    return;

  RunStatsTimerAtInterval(
      base::TimeDelta::FromMilliseconds(kStartingStatsDelayMs));
}

void MediaCapabilitiesReporter::OnPaused() {
  DVLOG(2) << __func__;

  // Stop timer until playing resumes. Abandon any scheduled update as
  // UpdateStats() may otherwise restart the timer.
  stats_cb_timer_.AbandonAndStop();
}

void MediaCapabilitiesReporter::OnNaturalSizeChanged(
    const gfx::Size& natural_size) {
  DVLOG(2) << __func__;

  natural_size_ = natural_size;
  recorder_ptr_->StartNewRecord(video_config_.profile(), natural_size_,
                                last_observed_fps_);
}

void MediaCapabilitiesReporter::OnVideoConfigChanged(
    const VideoDecoderConfig& video_config) {
  DVLOG(2) << __func__;
  video_config_ = video_config;
  natural_size_ = video_config.natural_size();

  // Wait a second for things to stabilize. Config changes may trigger
  // little hiccups while the decoder is reset.
  RunStatsTimerAtInterval(base::TimeDelta::FromSeconds(1));

  // Force framerate detection. Once framerate stabilized, UpdateStats()
  // will call the recorder_ptr_->StartNewRecord() with this latest config.
  last_observed_fps_ = 0;
}

int MediaCapabilitiesReporter::GetFrameRateBucket(PipelineStatistics stats) {
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

  // This will reset the timer clock to begin waiting for |interval| to lapse.
  stats_cb_timer_.Start(FROM_HERE, interval, this,
                        &MediaCapabilitiesReporter::UpdateStats);
}

void MediaCapabilitiesReporter::UpdateStats() {
  PipelineStatistics stats = get_pipeline_stats_cb_.Run();

  // No-op if no additional frames decoded since last check.
  if (stats.video_frames_decoded == last_frames_decoded_) {
    num_consecutive_no_decode_progress_++;

    // Relax timer if nothing is happening.
    if (num_consecutive_no_decode_progress_ == 10) {
      DVLOG(2) << __func__ << " No decode progress; slowing the timer";
      RunStatsTimerAtInterval(base::TimeDelta::FromMilliseconds(kSlowTimerMs));
    }
    return;
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

  int framerate = GetFrameRateBucket(stats);

  // When (re)initializing, the pipeline may momentarily return an average frame
  // duration of zero. Ignore it and wait for a real framerate.
  if (framerate == 0)
    return;

  // FIXME add logic to just bail (or wait a long while) if framerate keeps
  // swinging around (e.g. nest.com)
  if (framerate != last_observed_fps_) {
    DVLOG(2) << __func__ << " fps changed: " << last_observed_fps_ << " -> "
             << framerate;
    last_observed_fps_ = framerate;
    num_stable_fps_samples_ = 1;
    num_consecutive_fps_changes_++;

    // Increase the timer frequency to quickly stabilize framerate. 3x the
    // frame duration is used as this should be enough for a few more frames to
    // be decoded, while also being much faster (for typical framerates) than
    // the regular stats polling interval.
    RunStatsTimerAtInterval(3 * stats.video_frame_duration_average);
    return;
  } else {
    num_consecutive_fps_changes_ = 0;
    num_stable_fps_samples_++;

    // Wait for steady framerate to begin recording stats.
    if (num_stable_fps_samples_ < kRequiredStableFpsSamples) {
      DVLOG(2) << __func__ << " fps held, awaiting stable ("
               << num_stable_fps_samples_ << ")";
      return;
    } else if (num_stable_fps_samples_ == kRequiredStableFpsSamples) {
      // FPS is locked in. Start a new record, and update reporting state/timer.
      recorder_ptr_->StartNewRecord(video_config_.profile(), natural_size_,
                                    last_observed_fps_);
      RunStatsTimerAtInterval(reporting_interval_);
      frames_decoded_offset_ = stats.video_frames_decoded;
      frames_dropped_offset_ = stats.video_frames_dropped;
    }
  }

  uint32_t frames_decoded = stats.video_frames_decoded - frames_decoded_offset_;
  uint32_t frames_dropped = stats.video_frames_dropped - frames_dropped_offset_;
  DVLOG(2) << __func__ << " Recording fps:" << framerate
           << " dropped:" << frames_dropped << "/" << frames_decoded;
  recorder_ptr_->UpdateRecord(frames_decoded, frames_dropped);
}

}  // namespace media