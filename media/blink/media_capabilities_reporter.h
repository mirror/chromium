// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BLINK_MEDIA_CAPABILITIES_REPORTER_H_
#define MEDIA_BLINK_MEDIA_CAPABILITIES_REPORTER_H_

#include "base/memory/ptr_util.h"
#include "base/time/default_tick_clock.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "media/base/pipeline_status.h"
#include "media/base/video_decoder_config.h"
#include "media/blink/media_blink_export.h"
#include "media/mojo/interfaces/media_capabilities_recorder.mojom.h"

#include <memory>

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
                            const VideoDecoderConfig& video_config,
                            std::unique_ptr<base::TickClock> tick_clock =
                                base::MakeUnique<base::DefaultTickClock>());
  ~MediaCapabilitiesReporter();

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

  // FIXME
  // Should be called for any variety of remoting (e.g. remote rendering as
  // well as android-cast remoting).
  // void OnRemotingChanged(bool is_remoting);

 private:
  // Friends so it can see the static constants and inspect when the timer is
  // running / should be running.
  friend class MediaCapabilitiesReporterTest;

  // Timer interval for recording stats when framerate and other stream
  // properties are steady.
  static const int kRecordingIntervalMs = 2000;

  // Number of consecutive samples that must bucket to the same framerate in
  // order for framerate to be considered "stable" enough to start recording
  // stats.
  static const int kRequiredStableFpsSamples = 5;

  // Limits the number attempts to detect stable framerate. When this limit is
  // reached, we will give up until some stream property (e.g. decoder config)
  // changes before trying again. We do not wish to record stats for variable
  // framerate content.
  static const int kMaxUnstableFpsChanges = 10;

  // Framerates must remain stable for a duration greater than this amount to
  // avoid being classified as a "tiny fps window". See |kMaxTinyFpsWindows|.
  static const int kTinyFpsWindowMs = 5000;

  // Limits the number of consecutive "tiny fps windows", as defined by
  // |kTinyFpsWindowMs|. If this limit is met, we will give up until some stream
  // property (e.g. decoder config) changes before trying again. We do not wish
  // to record stats for variable framerate content.
  static const int kMaxTinyFpsWindows = 5;

  // Main driver of report updates. Queries |get_pipeline_stats_cb_| for the
  // latest stats. Detects framerate changes and playback stalls. Sends stats
  // the MediaCapabilitiesRecorder (browser process) for saving. Should only be
  // called when ShouldBeReporting() == true.
  void UpdateStats();

  // Round |raw_{size|fps}| to the nearest "bucket". Sizes/framerates in the
  // same bucket should have nearly identical decode performance
  // characteristics. Bucketing helps avoid fragmentation of recorded stats.
  gfx::Size GetSizeBucket(gfx::Size raw_size);
  int GetFpsBucket(double raw_fps);

  // Run |stats_cb_timer_| at the specified millisecond interval. If the timer
  // is already running, any existing callbacks will be canceled/delayed until
  // |milliseconds| has elapsed.
  void RunStatsTimerAtIntervalMs(double milliseconds);

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

  // Returns true if the |stats_timer_cb_| should be running. Should be called
  // after any state change (e.g. |is_playing_|) as a check on whether to start
  // the the timer.
  bool ShouldBeReporting() const;

  // Pointer to the remote recorder. The recorder runs in the browser process
  // and finalizes the record in the event of fast render process tear down.
  mojom::MediaCapabilitiesRecorderPtr recorder_ptr_;

  // Callback for retrieving playback statistics.
  GetPipelineStatsCB get_pipeline_stats_cb_;

  // Latest video decoder config, provided at construction or by
  // OnVideoConfigChanged(). Used to determine the codec, profile, and natural
  // size. Note that |video_config_.natural_size()| may differ slightly from the
  // stored |natural_size_| due to size bucketing. See GetSizeBucket()
  VideoDecoderConfig video_config_;

  // Last natural size, either extracted *and bucketed* (See GetSizeBucket())
  // from the latest |video_config_|, or from OnNaturalSizechanged(). The
  // dimensions of this size will always be rounded to the nearest size bucket.
  gfx::Size natural_size_;

  // Clock for |stats_cb_timer_| and getting current tick count (NowTicks()).
  // Tests may supply a mock clock via the constructor.
  std::unique_ptr<base::TickClock> tick_clock_;

  // Timer for all stats callbacks. Timer interval will be dynamically set based
  // on state of reporter. See calls to RunStatsTimerAtIntervalMs().
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
  bool is_backgrounded_ = false;
};

}  // namespace media

#endif  // MEDIA_BLINK_MEDIA_CAPABILITIES_REPORTER_H_