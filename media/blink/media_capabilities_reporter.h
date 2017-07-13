// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BLINK_MEDIA_CAPABILITIES_REPORTER_H_
#define MEDIA_BLINK_MEDIA_CAPABILITIES_REPORTER_H_

#include "base/time/time.h"
#include "base/timer/timer.h"
#include "media/base/pipeline_status.h"
#include "media/base/video_decoder_config.h"
#include "media/blink/media_blink_export.h"
#include "media/mojo/interfaces/media_capabilities_recorder.mojom.h"

namespace media {

// Reports on playback smoothness and for a given video codec profile, natural
// size, and fps. When these properties change the current report will be
// finalized and a new report will be started. Ongoing reports are also
// finalized at destruction and process tear-down.
class MEDIA_BLINK_EXPORT MediaCapabilitiesReporter {
 public:
  using GetPipelineStatsCB = base::Callback<PipelineStatistics(void)>;

  MediaCapabilitiesReporter(mojom::MediaCapabilitiesRecorderPtr recorder_ptr,
                            const GetPipelineStatsCB& get_pipeline_stats_cb,
                            const VideoDecoderConfig& video_config);
  ~MediaCapabilitiesReporter();

  void OnPlaying();
  void OnPlaybackRateChanged(double rate);
  void OnNaturalSizeChanged(const gfx::Size& natural_size);
  void OnVideoConfigChanged(const VideoDecoderConfig& video_config);

  // FOR THESE, maybe don't signal the reporter - instead just nuke it in WMPI
  // void OnEncrypted(bool is_encrypted);
  // Should be called for any variety of remoting (e.g. remote rendering as
  // well as android-cast remoting).
  // void OnRemotingChanged(bool is_remoting);

  // TODO:
  // OnBackgroundingChanged(bool is_backgrounded)

 private:
  void UpdateCapabilitiesReport();

  mojom::MediaCapabilitiesRecorderPtr recorder_ptr_;
  GetPipelineStatsCB get_pipeline_stats_cb_;
  VideoDecoderConfig video_config_;
  gfx::Size natural_size_;
  int frames_decoded_;
  int frames_dropped_;
  int avg_frame_duration_;
  int frames_power_efficient_;

  base::TimeDelta reporting_interval_ = base::TimeDelta::FromSeconds(5);

  base::RepeatingTimer reporting_timer_;
};

}  // namespace media

#endif  // MEDIA_BLINK_MEDIA_CAPABILITIES_REPORTER_H_