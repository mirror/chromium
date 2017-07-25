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

  static const int kFrameRateBuckets[];

  void OnPlaying();
  void OnPaused();
  void OnHidden();
  void OnShown();
  void OnNaturalSizeChanged(const gfx::Size& natural_size);
  void OnVideoConfigChanged(const VideoDecoderConfig& video_config);

  // NOTE: We do not listen for playback rate changes. These implicitly change
  // the framerate and surface via PipelineStatistics'
  // video_frame_duration_average.
  // TODO(chcunningham): Find a way to enforce this...

  // FOR THESE, maybe don't signal the reporter - instead just nuke it in WMPI
  // void OnEncrypted(bool is_encrypted);
  // Should be called for any variety of remoting (e.g. remote rendering as
  // well as android-cast remoting).
  // void OnRemotingChanged(bool is_remoting);

 private:
  void UpdateStats();

  int GetFrameRateBucket(const PipelineStatistics& stats) const;

  void RunStatsTimerAtInterval(base::TimeDelta interval);

  // Called to begin a new report following changes to stream metadata (e.g.
  // natural size). Arguments used to update |freames_decoded_offset_| and
  // |frames_dropped_offset_| so that frame counts for this report begin at 0.
  void StartNewRecord(uint32_t frames_decoded_offset,
                      uint32_t frames_dropped_offset);

  // Reset framerate tracking state to force a fresh attempt at detection. When
  // a stable framerate is successfully detected, UpdateStats() will begin a new
  // record will begin with the detected framerate. Note: callers must
  // separately ensure the |stats_cb_timer_| is running for framerate detection
  // to occur.
  void ResetFramerateState();

  // Called by UpdateStats() to verify decode is progressing and sanity check
  // decode/dropped frame counts. Will manage decoded/dropped frame state and
  // relax timer when no decode progress is made. Returns true iff decode is
  // progressing.
  bool AssessDecodeProgress(const PipelineStatistics& stats);

  // Called by UpdateStats() to do framerate detection. Will manage framerate
  // state, stats timer, and will start new capabilities records when framerate
  // changes. Returns true iff framerate is stable.
  bool AssessFramerateStability(const PipelineStatistics& stats);

  // Checks state like |is_playing_| and |is_backgrounded_| to determine whether
  // |stats_cb_timer_| should be running.
  bool ShouldBeReporting() const;

  mojom::MediaCapabilitiesRecorderPtr recorder_ptr_;
  GetPipelineStatsCB get_pipeline_stats_cb_;

  VideoDecoderConfig video_config_;

  gfx::Size natural_size_;

  // Interval to use for reporting. |stats_cb_timer_| will fire at this interval
  // once we reach stable FPS.
  base::TimeDelta reporting_interval_ = base::TimeDelta::FromSeconds(2);

  // Timer for all stats callbacks. The timer interval will initially vary as
  // we build up stats to determine stable FPS to begin reporting. Once
  // reporting starts, the timer interval is decided by |reporting_interval_|.
  base::RepeatingTimer stats_cb_timer_;

  // Latest frame duration bucketed into one of kFrameRateBuckets.
  int last_observed_fps_ = 0;

  // Count of consecutive samples with frame duration matching
  // |last_observed_fps_|.
  int num_stable_fps_samples_ = 0;

  // Count of consecutive samples with frame duration NOT matching
  // |last_obseved_fps_|. Used to throttle/limit attempts to stabilize FPS since
  // some videos may have variable framerate.
  int num_unstable_fps_changes_ = 0;

  // Count of consecutive stable FPS windows, where stable FPS was detected but
  // it lasted for a very short duration before changing again.
  int num_consecutive_tiny_fps_windows_ = 0;

  // True whenever we fail to determine a stable FPS, or if windows of stable
  // FPS are too tiny to be useful.
  bool fps_stabilization_failed_ = false;

  // Tick time of the most recent FPS stabilization. When FPS changes, this time
  // is compared to TimeTicks::Now() to approximate the duration of the last
  // stable FPS window.
  base::TimeTicks last_fps_stabilized_ticks_;

  // Count of consecutive stats callbacks where no progress was observed. Used
  // to throttle stats updates.
  int num_consecutive_no_decode_progress_ = 0;

  // Count of frames decoded observed in last pipeline stats update. Used to
  // check whether decode/playback has actually advanced.
  uint32_t last_frames_decoded_ = 0;

  // Count of frames dropped observed in last pipeline stats update. Used to
  // verify that count never decreases.
  uint32_t last_frames_dropped_ = 0;

  // Notes the number of frames decoded at the start of the current video
  // configuration (profile, resolution, fps). Should be subtracted from
  // pipeline frames decoded stats before sending to recorder.
  uint32_t frames_decoded_offset_ = 0;

  // Notes the number of frames dropped at the start of the current video
  // configuration (profile, resolution, fps). Should be subtracted from
  // pipeline frames dropped stats before sending to recorder.
  uint32_t frames_dropped_offset_ = 0;

  // Set true by OnPlaying(), false by OnPaused(). We should not run the
  // |stats_cb_timer_| when not playing.
  bool is_playing_ = false;

  // Set true by OnHidden(), false by OnVisible(). We should not run the
  // |stats_cb_timer_| when player is backgrounded.
  // FIXME - needed at construction?
  bool is_backgrounded_ = false;
};

}  // namespace media

#endif  // MEDIA_BLINK_MEDIA_CAPABILITIES_REPORTER_H_