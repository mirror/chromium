// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/media_capabilities_reporter.h"
#include "base/logging.h"

namespace media {
MediaCapabilitiesReporter::MediaCapabilitiesReporter(
    mojom::MediaCapabilitiesRecorderPtr recorder_ptr,
    const GetPipelineStatsCB& get_pipeline_stats_cb,
    const VideoDecoderConfig& video_config)
    : recorder_ptr_(std::move(recorder_ptr)),
      get_pipeline_stats_cb_(get_pipeline_stats_cb),
      video_config_(video_config),
      natural_size_(video_config.natural_size()),
      frames_decoded_(0),
      frames_dropped_(0),
      avg_frame_duration_(0),
      frames_power_efficient_(0) {}

MediaCapabilitiesReporter::~MediaCapabilitiesReporter() {
  recorder_ptr_->FinalizeRecord();
}

void MediaCapabilitiesReporter::OnPlaying() {
  if (reporting_timer_.IsRunning())
    return;

  LOG(ERROR) << __func__ << " starting timer";
  reporting_timer_.Start(FROM_HERE, reporting_interval_, this,
                         &MediaCapabilitiesReporter::UpdateCapabilitiesReport);
}

void MediaCapabilitiesReporter::OnPlaybackRateChanged(double rate) {
  LOG(ERROR) << __func__ << " not impl";
}

void MediaCapabilitiesReporter::OnNaturalSizeChanged(
    const gfx::Size& natural_size) {
  LOG(ERROR) << __func__ << " not impl";
}

void MediaCapabilitiesReporter::OnVideoConfigChanged(
    const VideoDecoderConfig& video_config) {
  LOG(ERROR) << __func__ << " not impl";
  // for config change, wait some number of seconds before doing more stuff
}

void MediaCapabilitiesReporter::UpdateCapabilitiesReport() {
  // For duration, consider some hysteresis / min-number-of-frames to
  // establish steady state. Changes should be significant (e.g. 10%) and hold
  // for some number of frames.

  PipelineStatistics stats = get_pipeline_stats_cb_.Run();
  LOG(ERROR) << __func__ << " " << stats.video_frames_dropped << "/"
             << stats.video_frames_decoded;

  recorder_ptr_->UpdateRecord(stats.video_frames_decoded,
                              stats.video_frames_dropped);

  frames_decoded_ = -1;
  frames_dropped_ = -1;
  avg_frame_duration_ = -1;
  frames_power_efficient_ = -1;
}

}  // namespace media