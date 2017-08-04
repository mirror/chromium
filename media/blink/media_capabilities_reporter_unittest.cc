// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/media_capabilities_reporter.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/time/time.h"
#include "media/base/media_util.h"
#include "media/base/video_codecs.h"
#include "media/base/video_types.h"
#include "media/mojo/interfaces/media_capabilities_recorder.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/rect.h"

using ::testing::Invoke;
using ::testing::Return;
using ::testing::_;

namespace media {

const VideoCodec kDefaultCodec = kCodecVP9;
const VideoCodecProfile kDefaultProfile = VP9PROFILE_PROFILE0;
const int kDefaultHeight = 480;
const int kDefaultWidth = 640;
const int kDefaultFps = 30;
const int kMillisecondsPerSecond = 1000;
const int kDecodeCountIncrement = 20;
const int kDroppedCountIncrement = 1;

VideoDecoderConfig MakeVideoConfig(VideoCodec codec,
                                   VideoCodecProfile profile,
                                   gfx::Size natural_size) {
  gfx::Size coded_size = natural_size;
  gfx::Rect visible_rect(coded_size.width(), coded_size.height());
  return VideoDecoderConfig(codec, profile, PIXEL_FORMAT_YV12, COLOR_SPACE_JPEG,
                            coded_size, visible_rect, natural_size,
                            EmptyExtraData(), Unencrypted());
}

VideoDecoderConfig MakeDefaultVideoConfig() {
  return MakeVideoConfig(kDefaultCodec, kDefaultProfile,
                         gfx::Size(kDefaultWidth, kDefaultHeight));
}

PipelineStatistics MakeStats(int frames_decoded, int frames_dropped, int fps) {
  // Will initialize members with reasonable defaults.
  PipelineStatistics stats;
  stats.video_frames_decoded = frames_decoded;
  stats.video_frames_dropped = frames_dropped;
  stats.video_frame_duration_average = base::TimeDelta::FromSecondsD(1.0 / fps);
  return stats;
}

class RecordInterceptor : public mojom::MediaCapabilitiesRecorder {
 public:
  RecordInterceptor() {}
  ~RecordInterceptor() override {}

  MOCK_METHOD3(StartNewRecord,
               void(VideoCodecProfile profile,
                    const gfx::Size& natural_size,
                    int frames_per_sec));

  MOCK_METHOD2(UpdateRecord, void(int frames_decoded, int frames_dropped));
  MOCK_METHOD0(FinalizeRecord, void());
};

class MediaCapabilitiesReporterTest : public ::testing::Test {
 public:
  MediaCapabilitiesReporterTest()
      : kDefaultSize_(kDefaultWidth, kDefaultHeight) {}
  ~MediaCapabilitiesReporterTest() override {}

  void SetUp() override {
    // Do this first. Lots of pieces depend on the task runner.
    task_runner_ = new base::TestMockTimeTaskRunner();
    message_loop_.SetTaskRunner(task_runner_);

    // Make reporter with default configuration. Connects RecordInterceptor as
    // remote mojo MediaCapabilitiesRecorder.
    MakeReporter();

    // Start each test with no decodes, no drops, and steady framerate.
    pipeline_decoded_frames_ = 0;
    pipeline_dropped_frames_ = 0;
    pipeline_framerate_ = kDefaultFps;
  }

  void TearDown() override {
    // Break the IPC connection if reporter still around.
    if (reporter_.get()) {
      EXPECT_CALL(*interceptor_, FinalizeRecord());
      reporter_.reset();
    }

    // Run task runner to have Mojo cleanup interceptor_.
    task_runner_->RunUntilIdle();
  }

  PipelineStatistics MakeAdvancingDecodeStats() {
    pipeline_decoded_frames_ += kDecodeCountIncrement;
    pipeline_dropped_frames_ += kDroppedCountIncrement;
    return MakeStats(pipeline_decoded_frames_, pipeline_dropped_frames_,
                     pipeline_framerate_);
  }

  // Peek at what MakeAdvancingDecodeStats() will return next without advancing
  // the tracked counts.
  PipelineStatistics PeekNextDecodeStats() const {
    return MakeStats(pipeline_decoded_frames_ + kDecodeCountIncrement,
                     pipeline_dropped_frames_ + kDroppedCountIncrement,
                     pipeline_framerate_);
  }

 protected:
  enum FpsStabiliaztionSpeed { FAST_STABILIZE_FPS, SLOW_STABILIZE_FPS };

  void SetupRecordInterceptor(mojom::MediaCapabilitiesRecorderPtr* recorder_ptr,
                              RecordInterceptor** interceptor) {
    // Capture a the interceptor pointer for verifying recorder calls. Lifetime
    // will be managed by the |recorder_ptr|.
    *interceptor = new RecordInterceptor();

    // Bind interceptor as the MediaCapabilitiesRecorder.
    mojom::MediaCapabilitiesRecorderRequest request =
        mojo::MakeRequest(recorder_ptr);
    mojo::MakeStrongBinding(base::WrapUnique(*interceptor),
                            mojo::MakeRequest(recorder_ptr));
    EXPECT_TRUE(recorder_ptr->is_bound());
  }

  void MakeReporter() {
    mojom::MediaCapabilitiesRecorderPtr recorder_ptr;
    SetupRecordInterceptor(&recorder_ptr, &interceptor_);

    reporter_ = base::MakeUnique<MediaCapabilitiesReporter>(
        std::move(recorder_ptr),
        base::Bind(&MediaCapabilitiesReporterTest::GetPipelineStatsCB,
                   base::Unretained(this)),
        MakeDefaultVideoConfig(), task_runner_->GetMockTickClock());
  }

  void FastForwardMsec(double milliseconds) {
    task_runner_->FastForwardBy(
        base::TimeDelta::FromMillisecondsD(milliseconds));
  }

  bool ShouldBeReporting() const { return reporter_->ShouldBeReporting(); }

  const VideoDecoderConfig& CurrentVideoConfig() const {
    return reporter_->video_config_;
  }

  double CurrentStatsCbIntervalMsec() const {
    return reporter_->stats_cb_timer_.GetCurrentDelay().InMillisecondsF();
  }

  int CurrentStableFpsSamples() const {
    return reporter_->num_stable_fps_samples_;
  }

  // Call at the start of tests to stabilize framerate.
  // Preconditions:
  //  1) Reporter should not already be in playing state
  //  2) Playing should unblock the reporter to begin reporting (e.g. not in
  //     hidden state)
  //  3) No progress made yet toward stabilizing framerate.
  void StartPlayingAndStabilizeFramerate() {
    DCHECK(!reporter_->is_playing_);
    DCHECK_EQ(reporter_->num_stable_fps_samples_, 0);

    // Setup stats callback to provide steadily advancing decode stats with a
    // constant framerate.
    ON_CALL(*this, GetPipelineStatsCB())
        .WillByDefault(Invoke(
            this, &MediaCapabilitiesReporterTest::MakeAdvancingDecodeStats));

    // On playing should start timer at recording interval.
    reporter_->OnPlaying();
    DCHECK(ShouldBeReporting());

    // OnPlaying starts the timer at the recording interval. Expect first stats
    // CB when that interval has elapsed.
    EXPECT_CALL(*this, GetPipelineStatsCB());
    FastForwardMsec(kRecordingIntervalMs);

    StabilizeFramerateAndStartNewRecord(kDefaultProfile, kDefaultSize_,
                                        kDefaultFps);
  }

  // Preconditions:
  //  1. Do not call if framerate already stable (know what you're testing).
  //  2. Only call with GetPipelineStatsCB configured to return
  //     progressing decode stats with a steady framerate.
  void StabilizeFramerateAndStartNewRecord(
      VideoCodecProfile profile,
      gfx::Size natural_size,
      int frames_per_sec,
      FpsStabiliaztionSpeed fps_timer_speed = FAST_STABILIZE_FPS) {
    base::TimeDelta last_frame_duration = kNoTimestamp;
    uint32_t last_decoded_frames = 0;

    while (CurrentStableFpsSamples() < kRequiredStableFpsSamples) {
      PipelineStatistics next_stats = PeekNextDecodeStats();

      // Sanity check that the stats callback is progressing decode.
      DCHECK_GT(next_stats.video_frames_decoded, last_decoded_frames);
      last_decoded_frames = next_stats.video_frames_decoded;

      // Sanity check that the stats callback is providing steady fps.
      if (last_frame_duration != kNoTimestamp) {
        DCHECK_EQ(next_stats.video_frame_duration_average, last_frame_duration);
      }
      last_frame_duration = next_stats.video_frame_duration_average;

      // The final iteration stabilizes framerate and starts a new record.
      if (CurrentStableFpsSamples() == kRequiredStableFpsSamples - 1) {
        EXPECT_CALL(*interceptor_,
                    StartNewRecord(profile, natural_size, frames_per_sec));
      }

      if (fps_timer_speed == FAST_STABILIZE_FPS) {
        // Generally FPS is stabilized with a timer of ~3x the average frame
        // duration.
        base::TimeDelta frame_druation =
            base::TimeDelta::FromSecondsD(1.0 / pipeline_framerate_);
        EXPECT_EQ(std::round(CurrentStatsCbIntervalMsec()),
                  std::round(frame_druation.InMillisecondsF() * 3));
      } else {
        // If the playback is struggling we will do it more slowly to avoid
        // firing high frequency timers indefinitely. Store constant as a local
        // to workaround linking confusion.
        DCHECK_EQ(fps_timer_speed, SLOW_STABILIZE_FPS);
        int recording_interval_ms = kRecordingIntervalMs;
        EXPECT_EQ(CurrentStatsCbIntervalMsec(), recording_interval_ms);
      }

      EXPECT_CALL(*this, GetPipelineStatsCB());
      FastForwardMsec(CurrentStatsCbIntervalMsec());
    }
  }

  void AdvanceTimeAndVerifyRecordUpdate(int decoded_frames_offset,
                                        int dropped_frames_offset) {
    // Record updates should always occur at recording interval. Store to local
    // variable to workaround linker confusion with test macros.
    const int recording_interval_ms = kRecordingIntervalMs;
    EXPECT_EQ(recording_interval_ms, CurrentStatsCbIntervalMsec());

    PipelineStatistics next_stats = PeekNextDecodeStats();

    // Decode stats must be advancing for record updates to be expected. Dropped
    // frames should at least not move backward.
    EXPECT_GT(next_stats.video_frames_decoded, pipeline_decoded_frames_);
    EXPECT_GE(next_stats.video_frames_dropped, pipeline_dropped_frames_);

    // Verify that UpdateRecord calls come at the recording interval with
    // correct values.
    EXPECT_CALL(*this, GetPipelineStatsCB());
    EXPECT_CALL(
        *interceptor_,
        UpdateRecord(next_stats.video_frames_decoded - decoded_frames_offset,
                     next_stats.video_frames_dropped - dropped_frames_offset));
    FastForwardMsec(kRecordingIntervalMs);
  }

  MOCK_METHOD0(GetPipelineStatsCB, PipelineStatistics());

  // These track the last values returned by MakeAdvancingDecodeStats(). See
  // SetUp() for initialization.
  uint32_t pipeline_decoded_frames_;
  uint32_t pipeline_dropped_frames_;
  uint32_t pipeline_framerate_;

  // Placed as a class member to avoid static initialization costs.
  const gfx::Size kDefaultSize_;

  // Put first so it will be destructed *last*. We must let users of the
  // message loop (e.g. reporter_) destruct before destructing the loop itself.
  base::MessageLoop message_loop_;

  // Task runner that allows for manual advancing of time. Instantiated and
  // used by message_loop_ in Setup().
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;

  // Points to the interceptor that acts as a MediaCapabilitiesRecorder. The
  // object is owned by MediaCapabilitiesRecorderPtr, which is itself owned by
  // |reporter_|.
  RecordInterceptor* interceptor_ = nullptr;

  // The MediaCapabilitiesReporter being tested.
  std::unique_ptr<MediaCapabilitiesReporter> reporter_;

  // Redefined constants in fixture for easy access from tests. See
  // media_capabilities_reporter.h for documentation.
  static const int kRecordingIntervalMs =
      MediaCapabilitiesReporter::kRecordingIntervalMs;
  static const int kRequiredStableFpsSamples =
      MediaCapabilitiesReporter::kRequiredStableFpsSamples;
  static const int kMaxUnstableFpsChanges =
      MediaCapabilitiesReporter::kMaxUnstableFpsChanges;
  static const int kTinyFpsWindowMs =
      MediaCapabilitiesReporter::kTinyFpsWindowMs;
  static const int kMaxTinyFpsWindows =
      MediaCapabilitiesReporter::kMaxTinyFpsWindows;
};

TEST_F(MediaCapabilitiesReporterTest, RecordWhilePlaying) {
  StartPlayingAndStabilizeFramerate();

  // Framerate is now stable! Recorded stats should be offset by the values
  // last provided to GetPipelineStatsCB.
  uint32_t decoded_offset = pipeline_decoded_frames_;
  uint32_t dropped_offset = pipeline_dropped_frames_;

  // Verify that UpdateRecord calls come at the recording interval with
  // correct values.
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);

  // Once more for good measure.
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);
}

TEST_F(MediaCapabilitiesReporterTest, RecordingStopsWhenPaused) {
  StartPlayingAndStabilizeFramerate();

  // Framerate is now stable! Recorded stats should be offset by the values
  // last provided to GetPipelineStatsCB.
  uint32_t decoded_offset = pipeline_decoded_frames_;
  uint32_t dropped_offset = pipeline_dropped_frames_;

  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);

  // When paused, expect no stats callbacks and no record updates.
  reporter_->OnPaused();
  EXPECT_FALSE(ShouldBeReporting());
  EXPECT_CALL(*this, GetPipelineStatsCB()).Times(0);
  EXPECT_CALL(*interceptor_, UpdateRecord(_, _)).Times(0);
  // Advance a few recording intervals just to be sure.
  FastForwardMsec(kRecordingIntervalMs * 3);

  // Verify callbacks and record updates resume when playing again. No changes
  // to the stream during pause, so no need to re-stabilize framerate. Offsets
  // for stats count are still valid.
  reporter_->OnPlaying();
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);
}

TEST_F(MediaCapabilitiesReporterTest, RecordingStopsWhenHidden) {
  StartPlayingAndStabilizeFramerate();

  // Framerate is now stable! Recorded stats should be offset by the values
  // last provided to GetPipelineStatsCB.
  uint32_t decoded_offset = pipeline_decoded_frames_;
  uint32_t dropped_offset = pipeline_dropped_frames_;

  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);

  // When hidden, expect no stats callbacks and no record updates.
  reporter_->OnHidden();
  EXPECT_FALSE(ShouldBeReporting());
  EXPECT_CALL(*this, GetPipelineStatsCB()).Times(0);
  EXPECT_CALL(*interceptor_, UpdateRecord(_, _)).Times(0);
  // Advance a few recording intervals just to be sure.
  FastForwardMsec(kRecordingIntervalMs * 3);

  // Verify updates resume when visible again. Dropped frame stats are not valid
  // while hidden, so expect a new record to begin. GetPipelineStatsCB will be
  // called to update offsets to ignore stats while hidden.
  EXPECT_CALL(*this, GetPipelineStatsCB());
  EXPECT_CALL(*interceptor_,
              StartNewRecord(kDefaultProfile, kDefaultSize_, kDefaultFps));
  reporter_->OnShown();

  // Update offsets and verify record updates resume as time advances.
  decoded_offset = pipeline_decoded_frames_;
  dropped_offset = pipeline_dropped_frames_;
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);
}

TEST_F(MediaCapabilitiesReporterTest, RecordingStopsWhenNoDecodeProgress) {
  StartPlayingAndStabilizeFramerate();

  // Framerate is now stable! Recorded stats should be offset by the values
  // last provided to GetPipelineStatsCB.
  uint32_t decoded_offset = pipeline_decoded_frames_;
  uint32_t dropped_offset = pipeline_dropped_frames_;

  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);

  // Freeze decode stats at current values, simulating network underflow.
  ON_CALL(*this, GetPipelineStatsCB())
      .WillByDefault(
          Return(MakeStats(pipeline_decoded_frames_, pipeline_dropped_frames_,
                           pipeline_framerate_)));

  // Verify record updates stop while decode is not progressing. Fast forward
  // through several recording intervals to be sure we never call UpdateRecord.
  EXPECT_CALL(*this, GetPipelineStatsCB()).Times(3);
  EXPECT_CALL(*interceptor_, UpdateRecord(_, _)).Times(0);
  FastForwardMsec(kRecordingIntervalMs * 3);

  // Resume progressing decode!
  ON_CALL(*this, GetPipelineStatsCB())
      .WillByDefault(Invoke(
          this, &MediaCapabilitiesReporterTest::MakeAdvancingDecodeStats));

  // Verify record updates resume.
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);
}

TEST_F(MediaCapabilitiesReporterTest, ThrottleFpsTimerIfNoDecodeProgress) {
  // Setup stats callback to provide steadily advancing decode stats with a
  // constant framerate.
  ON_CALL(*this, GetPipelineStatsCB())
      .WillByDefault(Invoke(
          this, &MediaCapabilitiesReporterTest::MakeAdvancingDecodeStats));

  // On playing should start timer at recording interval.
  reporter_->OnPlaying();
  EXPECT_TRUE(ShouldBeReporting());

  // OnPlaying starts the timer at the recording interval. Expect first stats
  // CB when that interval has elapsed. This stats callback provides the first
  // fps sample.
  EXPECT_CALL(*this, GetPipelineStatsCB());
  FastForwardMsec(kRecordingIntervalMs);
  int stable_fps_samples = 1;

  // Now advance time to make it half way through framerate stabilization.
  double frame_duration_ms =
      (1.0 / pipeline_framerate_) * kMillisecondsPerSecond;
  for (; stable_fps_samples < kRequiredStableFpsSamples / 2;
       stable_fps_samples++) {
    // The timer runs at 3x the frame duration when detecting framerate to
    // quickly stabilize.
    EXPECT_CALL(*this, GetPipelineStatsCB());
    FastForwardMsec(frame_duration_ms * 3);
  }

  // With stabilization still ongoing, freeze decode progress by repeatedly
  // returning the same stats from before.
  ON_CALL(*this, GetPipelineStatsCB())
      .WillByDefault(
          Return(MakeStats(pipeline_decoded_frames_, pipeline_dropped_frames_,
                           pipeline_framerate_)));

  // Advance another fps detection interval to detect that no progress was made.
  // Verify this decreases timer frequency to standard reporting interval.
  const int recording_interval_ms = kRecordingIntervalMs;
  EXPECT_LT(CurrentStatsCbIntervalMsec(), recording_interval_ms);
  EXPECT_CALL(*this, GetPipelineStatsCB());
  FastForwardMsec(frame_duration_ms * 3);
  EXPECT_EQ(CurrentStatsCbIntervalMsec(), recording_interval_ms);

  // Verify stats updates continue to come in at recording interval. Verify no
  // calls to UpdateRecord because decode progress is still frozen. Fast forward
  // through several recording intervals to be sure nothing changes.
  EXPECT_CALL(*this, GetPipelineStatsCB()).Times(3);
  EXPECT_CALL(*interceptor_, UpdateRecord(_, _)).Times(0);
  FastForwardMsec(kRecordingIntervalMs * 3);

  // Un-freeze decode stats!
  ON_CALL(*this, GetPipelineStatsCB())
      .WillByDefault(Invoke(
          this, &MediaCapabilitiesReporterTest::MakeAdvancingDecodeStats));

  // Finish framerate stabilization with a slower timer frequency. The slower
  // timer is used to avoid firing high frequency timers indefinitely for
  // machines/networks that are struggling to keep up.
  StabilizeFramerateAndStartNewRecord(kDefaultProfile, kDefaultSize_,
                                      kDefaultFps, SLOW_STABILIZE_FPS);

  // Framerate is now stable! Recorded stats should be offset by the values
  // last provided to GetPipelineStatsCB.
  uint32_t decoded_offset = pipeline_decoded_frames_;
  uint32_t dropped_offset = pipeline_dropped_frames_;

  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);
}

TEST_F(MediaCapabilitiesReporterTest, FpsStabiliaztionFailed) {
  // Setup stats callback to provide steadily advancing decode stats with a
  // constant framerate.
  ON_CALL(*this, GetPipelineStatsCB())
      .WillByDefault(Invoke(
          this, &MediaCapabilitiesReporterTest::MakeAdvancingDecodeStats));

  // On playing should start timer.
  reporter_->OnPlaying();
  EXPECT_TRUE(ShouldBeReporting());

  // OnPlaying starts the timer at the recording interval. Expect first stats
  // CB when that interval has elapsed. This stats callback provides the first
  // fps sample.
  EXPECT_CALL(*this, GetPipelineStatsCB());

  // We should not start nor update a record while failing to detect fps.
  EXPECT_CALL(*interceptor_, UpdateRecord(_, _)).Times(0);
  EXPECT_CALL(*interceptor_, StartNewRecord(_, _, _)).Times(0);
  FastForwardMsec(kRecordingIntervalMs);
  int num_fps_samples = 1;

  // With FPS stabilization in-progress, keep alternating framerate to thwart
  // stabilization.
  for (; num_fps_samples < kMaxUnstableFpsChanges; num_fps_samples++) {
    pipeline_framerate_ =
        (pipeline_framerate_ == kDefaultFps) ? 2 * kDefaultFps : kDefaultFps;
    EXPECT_CALL(*this, GetPipelineStatsCB());
    FastForwardMsec(CurrentStatsCbIntervalMsec());
  }

  // Stabilization has now failed, so fast forwarding  by any amount will not
  // trigger new stats update callbacks.
  EXPECT_CALL(*this, GetPipelineStatsCB()).Times(0);
  FastForwardMsec(kRecordingIntervalMs * 10);

  // Pausing then playing does not kickstart reporting.
  reporter_->OnPaused();
  reporter_->OnPlaying();
  FastForwardMsec(kRecordingIntervalMs * 10);

  // Hide/Show does not kickstart reporting.
  // FIXME - ON shown is causing failures
  reporter_->OnHidden();
  reporter_->OnShown();
  FastForwardMsec(kRecordingIntervalMs * 10);
}

TEST_F(MediaCapabilitiesReporterTest, NewRecordStartsForSizeChange) {
  StartPlayingAndStabilizeFramerate();

  // Framerate is now stable! Recorded stats should be offset by the values
  // last provided to GetPipelineStatsCB.
  uint32_t decoded_offset = pipeline_decoded_frames_;
  uint32_t dropped_offset = pipeline_dropped_frames_;

  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);

  // Change the natural size.
  const gfx::Size size_720p(1280, 720);
  reporter_->OnNaturalSizeChanged(size_720p);

  // Next stats update will not cause a record update. We must first check
  // to see if the framerate changes and start a new record.
  EXPECT_CALL(*this, GetPipelineStatsCB());
  EXPECT_CALL(*interceptor_, UpdateRecord(_, _)).Times(0);
  FastForwardMsec(kRecordingIntervalMs);

  // A new record is started with the latest natural size as soon as the
  // framerate is confirmed (re-stabilized).
  StabilizeFramerateAndStartNewRecord(kDefaultProfile, size_720p, kDefaultFps);

  // Offsets should be adjusted so the new record starts at zero.
  decoded_offset = pipeline_decoded_frames_;
  dropped_offset = pipeline_dropped_frames_;

  // Stats callbacks and record updates should proceed as usual.
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);
}

TEST_F(MediaCapabilitiesReporterTest, NewRecordStartsForConfigChange) {
  StartPlayingAndStabilizeFramerate();

  // Framerate is now stable! Recorded stats should be offset by the values
  // last provided to GetPipelineStatsCB.
  uint32_t decoded_offset = pipeline_decoded_frames_;
  uint32_t dropped_offset = pipeline_dropped_frames_;

  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);

  // Change the config to use profile 2.
  const gfx::Size kDefaultSize(kDefaultWidth, kDefaultHeight);
  auto new_config =
      MakeVideoConfig(kCodecVP9, VP9PROFILE_PROFILE2, kDefaultSize);
  EXPECT_FALSE(new_config.Matches(CurrentVideoConfig()));
  reporter_->OnVideoConfigChanged(new_config);

  // Next stats update will not cause a record update. We must first check
  // to see if the framerate changes and start a new record.
  EXPECT_CALL(*this, GetPipelineStatsCB());
  EXPECT_CALL(*interceptor_, UpdateRecord(_, _)).Times(0);
  FastForwardMsec(kRecordingIntervalMs);

  // A new record is started with the latest configuration as soon as the
  // framerate is confirmed (re-stabilized).
  StabilizeFramerateAndStartNewRecord(VP9PROFILE_PROFILE2, kDefaultSize,
                                      kDefaultFps);

  // Offsets should be adjusted so the new record starts at zero.
  decoded_offset = pipeline_decoded_frames_;
  dropped_offset = pipeline_dropped_frames_;

  // Stats callbacks and record updates should proceed as usual.
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);
}

TEST_F(MediaCapabilitiesReporterTest, NewRecordStartsForFpsChange) {
  StartPlayingAndStabilizeFramerate();

  // Framerate is now stable! Recorded stats should be offset by the values
  // last provided to GetPipelineStatsCB.
  uint32_t decoded_offset = pipeline_decoded_frames_;
  uint32_t dropped_offset = pipeline_dropped_frames_;

  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);

  // Change FPS to 2x current rate. Future calls to GetPipelineStats will
  // use this to compute frame duration.
  EXPECT_EQ(pipeline_framerate_, static_cast<uint32_t>(kDefaultFps));
  pipeline_framerate_ *= 2;

  // Next stats update will not cause a record update. It will instead begin
  // detection of the new framerate.
  EXPECT_CALL(*this, GetPipelineStatsCB());
  EXPECT_CALL(*interceptor_, UpdateRecord(_, _)).Times(0);
  FastForwardMsec(kRecordingIntervalMs);

  // A new record is started with the latest frames per second as soon as the
  // framerate is confirmed (re-stabilized).
  StabilizeFramerateAndStartNewRecord(kDefaultProfile, kDefaultSize_,
                                      kDefaultFps * 2);

  // Offsets should be adjusted so the new record starts at zero.
  decoded_offset = pipeline_decoded_frames_;
  dropped_offset = pipeline_dropped_frames_;

  // Stats callbacks and record updates should proceed as usual.
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);
}

// TESTS TO WRITE
// 8. FPS stabilization fails -> timer stops until config change
// 9. FPS stabilization tiny windows -> timer stops until config change
// 10. Recording stops when hidden, config/size change still processed though
// 11. fps stabiliaztion Timer slows if no decode progress.
// 12. resolution bucketing
// 13. framerate bucketing

}  // namespace media