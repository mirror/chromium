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
const double kDefaultFps = 30;
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

// Mock MediaCapabilitiesRecorder to verify reporter/recorder interactions.
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
  // Indicates expectations about rate of timer firing during FPS stabilization.
  enum FpsStabiliaztionSpeed {
    // Timer is expected to fire at rate of 3 * last observed average frame
    // duration. This is the default for framerate detection.
    FAST_STABILIZE_FPS,
    // Timer is expected to fire at a rate of kRecordingInterval. This will
    // occur
    // when decode progress stalls during framerate detection.
    SLOW_STABILIZE_FPS,
  };

  // Bind the RecordInterceptor to the request for a MediaCapabilitiesRecorder.
  // The interceptor serves as a mock recorder to verify reporter/recorder
  // interactions.
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

  // Inject mock objects and create a new |reporter_| to test.
  void MakeReporter() {
    mojom::MediaCapabilitiesRecorderPtr recorder_ptr;
    SetupRecordInterceptor(&recorder_ptr, &interceptor_);

    reporter_ = base::MakeUnique<MediaCapabilitiesReporter>(
        std::move(recorder_ptr),
        base::Bind(&MediaCapabilitiesReporterTest::GetPipelineStatsCB,
                   base::Unretained(this)),
        MakeDefaultVideoConfig(), task_runner_->GetMockTickClock());
  }

  // Fast forward the task runner (and associated tick clock) by |milliseconds|.
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

  gfx::Size GetSizeBucket(gfx::Size raw_size) {
    return reporter_->GetSizeBucket(raw_size);
  }

  int GetFpsBucket(double raw_fps) { return reporter_->GetFpsBucket(raw_fps); }

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

    // On playing should start timer at recording interval. Expect first stats
    // CB when that interval has elapsed.
    reporter_->OnPlaying();
    DCHECK(ShouldBeReporting());
    EXPECT_CALL(*this, GetPipelineStatsCB());
    FastForwardMsec(kRecordingIntervalMs);

    StabilizeFramerateAndStartNewRecord(kDefaultProfile, kDefaultSize_,
                                        kDefaultFps);
  }

  // Call just after detecting a change to framerate. |profile|, |natural_size|,
  // and |frames_per_sec| should match the call to StartNewRecord(...) once the
  // framerate is stabilized. |fps_timer_speed| indicates the expected timer
  // interval to be used during stabilization (see FpsStabiliaztionSpeed
  // definition).
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

  // Advances the task runner by a single recording interval and verifies that
  // the record is updated. The values provided to UpdateRecord(...)
  // should match values from PeekNextDecodeStates(...), minus the offsets of
  // |decoded_frames_offset| and |dropped_frames_offset|.
  // Preconditions:
  // - Should only be called during regular reporting (framerate stable,
  //   not in background, not paused).
  void AdvanceTimeAndVerifyRecordUpdate(int decoded_frames_offset,
                                        int dropped_frames_offset) {
    DCHECK(ShouldBeReporting());

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

  // Injected callback for fetching statistics. Each test will manage
  // expectations and return behavior.
  MOCK_METHOD0(GetPipelineStatsCB, PipelineStatistics());

  // These track the last values returned by MakeAdvancingDecodeStats(). See
  // SetUp() for initialization.
  uint32_t pipeline_decoded_frames_;
  uint32_t pipeline_dropped_frames_;
  double pipeline_framerate_;

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

  // Update offsets for new record and verify updates resume as time advances.
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
  auto new_config =
      MakeVideoConfig(kCodecVP9, VP9PROFILE_PROFILE2, kDefaultSize_);
  EXPECT_FALSE(new_config.Matches(CurrentVideoConfig()));
  reporter_->OnVideoConfigChanged(new_config);

  // Next stats update will not cause a record update. We must first check
  // to see if the framerate changes and start a new record.
  EXPECT_CALL(*this, GetPipelineStatsCB());
  EXPECT_CALL(*interceptor_, UpdateRecord(_, _)).Times(0);
  FastForwardMsec(kRecordingIntervalMs);

  // A new record is started with the latest configuration as soon as the
  // framerate is confirmed (re-stabilized).
  StabilizeFramerateAndStartNewRecord(VP9PROFILE_PROFILE2, kDefaultSize_,
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

TEST_F(MediaCapabilitiesReporterTest, FpsStabilizationFailed) {
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

  // Pausing then playing does not kickstart reporting. We assume framerate is
  // still variable.
  reporter_->OnPaused();
  reporter_->OnPlaying();
  FastForwardMsec(kRecordingIntervalMs * 10);

  // Hidden then shown does not kickstart reporting. We assume framerate is
  // still variable.
  reporter_->OnHidden();
  reporter_->OnShown();
  FastForwardMsec(kRecordingIntervalMs * 10);

  // Unlike the above, a config change suggests the stream itself has changed so
  // we should make a new attempt at detecting a stable FPS.
  VideoDecoderConfig new_config =
      MakeVideoConfig(kDefaultCodec, VP9PROFILE_PROFILE2, kDefaultSize_);
  EXPECT_FALSE(new_config.Matches(CurrentVideoConfig()));
  reporter_->OnVideoConfigChanged(new_config);
  EXPECT_CALL(*this, GetPipelineStatsCB());
  FastForwardMsec(kRecordingIntervalMs);

  // With |pipeline_framerate_| holding steady, FPS should stabilize. The new
  // record should indicate we're using VP9 Profile 2.
  StabilizeFramerateAndStartNewRecord(VP9PROFILE_PROFILE2, kDefaultSize_,
                                      pipeline_framerate_);

  // Framerate is now stable! Recorded stats should be offset by the values
  // last provided to GetPipelineStatsCB.
  uint32_t decoded_offset = pipeline_decoded_frames_;
  uint32_t dropped_offset = pipeline_dropped_frames_;

  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);
}

TEST_F(MediaCapabilitiesReporterTest, FpsStabilizationFailed_TinyWindows) {
  uint32_t decoded_offset = 0;
  uint32_t dropped_offset = 0;

  // Setup stats callback to provide steadily advancing decode stats.
  ON_CALL(*this, GetPipelineStatsCB())
      .WillByDefault(Invoke(
          this, &MediaCapabilitiesReporterTest::MakeAdvancingDecodeStats));

  // On playing should start timer at recording interval. Expect first stats
  // CB when that interval has elapsed.
  reporter_->OnPlaying();
  DCHECK(ShouldBeReporting());
  EXPECT_CALL(*this, GetPipelineStatsCB());
  FastForwardMsec(kRecordingIntervalMs);

  // Repeatedly stabilize, then change the FPS after single record updates to
  // create tiny windows.
  for (int i = 0; i < kMaxTinyFpsWindows; i++) {
    StabilizeFramerateAndStartNewRecord(kDefaultProfile, kDefaultSize_,
                                        pipeline_framerate_);

    // Framerate is now stable! Recorded stats should be offset by the values
    // last provided to GetPipelineStatsCB.
    decoded_offset = pipeline_decoded_frames_;
    dropped_offset = pipeline_dropped_frames_;

    AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);

    // Changing the framerate and fast forward to detect the change.
    pipeline_framerate_ =
        (pipeline_framerate_ == kDefaultFps) ? 2 * kDefaultFps : kDefaultFps;
    EXPECT_CALL(*this, GetPipelineStatsCB());
    FastForwardMsec(kRecordingIntervalMs);
  }

  // Verify no further stats updates are made because we've hit the maximum
  // number of tiny framerate windows.
  EXPECT_CALL(*this, GetPipelineStatsCB()).Times(0);
  EXPECT_CALL(*interceptor_, UpdateRecord(_, _)).Times(0);
  FastForwardMsec(kRecordingIntervalMs);

  // Pausing then playing does not kickstart reporting. We assume framerate is
  // still variable.
  reporter_->OnPaused();
  reporter_->OnPlaying();
  FastForwardMsec(kRecordingIntervalMs * 10);

  // Hidden then shown does not kickstart reporting. We assume framerate is
  // still variable.
  reporter_->OnHidden();
  reporter_->OnShown();
  FastForwardMsec(kRecordingIntervalMs * 10);

  // Unlike the above, a natural size change suggests the stream itself has
  // changed so we should make a new attempt at detecting a stable FPS.
  gfx::Size size_720p(1280, 720);
  reporter_->OnNaturalSizeChanged(size_720p);
  EXPECT_CALL(*this, GetPipelineStatsCB());
  FastForwardMsec(kRecordingIntervalMs);

  // With |pipeline_framerate_| holding steady, FPS stabilization should work.
  StabilizeFramerateAndStartNewRecord(kDefaultProfile, size_720p,
                                      pipeline_framerate_);

  // Framerate is now stable! Recorded stats should be offset by the values
  // last provided to GetPipelineStatsCB.
  decoded_offset = pipeline_decoded_frames_;
  dropped_offset = pipeline_dropped_frames_;

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

TEST_F(MediaCapabilitiesReporterTest, ConfigChangeStillProcessedWhenHidden) {
  StartPlayingAndStabilizeFramerate();

  // Framerate is now stable! Recorded stats should be offset by the values
  // last provided to GetPipelineStatsCB.
  uint32_t decoded_offset = pipeline_decoded_frames_;
  uint32_t dropped_offset = pipeline_dropped_frames_;

  // Verify that UpdateRecord calls come at the recording interval with
  // correct values.
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);

  // When hidden, expect no stats callbacks and no record updates. Advance a few
  // recording intervals just to be sure.
  reporter_->OnHidden();
  EXPECT_FALSE(ShouldBeReporting());
  EXPECT_CALL(*this, GetPipelineStatsCB()).Times(0);
  EXPECT_CALL(*interceptor_, UpdateRecord(_, _)).Times(0);
  FastForwardMsec(kRecordingIntervalMs * 3);

  // Config changes may still arrive when hidden and should not be dropped.
  // Change the config to use VP9 Profile 2.
  VideoDecoderConfig new_config =
      MakeVideoConfig(kDefaultCodec, VP9PROFILE_PROFILE2, kDefaultSize_);
  EXPECT_FALSE(new_config.Matches(CurrentVideoConfig()));
  reporter_->OnVideoConfigChanged(new_config);

  // Still, no record updates should be made until the the reporter is shown.
  FastForwardMsec(kRecordingIntervalMs * 3);

  // When shown, the reporting timer should start running again.
  reporter_->OnShown();
  EXPECT_CALL(*this, GetPipelineStatsCB());
  FastForwardMsec(kRecordingIntervalMs);

  // Framerate should be re-detected whenever the stream config changes. A new
  // record should be started using VP9 Profile 2 from the new config.
  StabilizeFramerateAndStartNewRecord(VP9PROFILE_PROFILE2, kDefaultSize_,
                                      kDefaultFps);

  // Update offsets for new record and verify updates resume as time advances.
  decoded_offset = pipeline_decoded_frames_;
  dropped_offset = pipeline_dropped_frames_;
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);
}

TEST_F(MediaCapabilitiesReporterTest, ConfigChangeStillProcessedWhenPaused) {
  StartPlayingAndStabilizeFramerate();

  // Framerate is now stable! Recorded stats should be offset by the values
  // last provided to GetPipelineStatsCB.
  uint32_t decoded_offset = pipeline_decoded_frames_;
  uint32_t dropped_offset = pipeline_dropped_frames_;

  // Verify that UpdateRecord calls come at the recording interval with
  // correct values.
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);

  // Pause and verify record updates stop.
  reporter_->OnPaused();
  EXPECT_FALSE(ShouldBeReporting());
  EXPECT_CALL(*this, GetPipelineStatsCB()).Times(0);
  EXPECT_CALL(*interceptor_, UpdateRecord(_, _)).Times(0);
  FastForwardMsec(kRecordingIntervalMs * 3);

  // Config changes are still possible when paused (e.g. user seeks to a new
  // config). Change the config to VP9 Profile 2.
  auto new_config =
      MakeVideoConfig(kCodecVP9, VP9PROFILE_PROFILE2, kDefaultSize_);
  EXPECT_FALSE(new_config.Matches(CurrentVideoConfig()));
  reporter_->OnVideoConfigChanged(new_config);

  // Playback is still paused, so reporting should be stopped.
  EXPECT_FALSE(ShouldBeReporting());
  EXPECT_CALL(*this, GetPipelineStatsCB()).Times(0);
  EXPECT_CALL(*interceptor_, UpdateRecord(_, _)).Times(0);
  FastForwardMsec(kRecordingIntervalMs * 3);

  // Upon playing, expect the new config to re-trigger framerate detection and
  // to begin a new record using VP9 Profile 2. Fast forward an initial
  // recording interval to pick up the config change.
  reporter_->OnPlaying();
  EXPECT_CALL(*this, GetPipelineStatsCB());
  FastForwardMsec(kRecordingIntervalMs);
  StabilizeFramerateAndStartNewRecord(VP9PROFILE_PROFILE2, kDefaultSize_,
                                      kDefaultFps);

  // Update offsets for new record and verify record updates.
  decoded_offset = pipeline_decoded_frames_;
  dropped_offset = pipeline_dropped_frames_;
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);
}

TEST_F(MediaCapabilitiesReporterTest, FpsBucketing) {
  StartPlayingAndStabilizeFramerate();
  EXPECT_EQ(kDefaultFps, pipeline_framerate_);

  // Framerate is now stable! Recorded stats should be offset by the values
  // last provided to GetPipelineStatsCB.
  uint32_t decoded_offset = pipeline_decoded_frames_;
  uint32_t dropped_offset = pipeline_dropped_frames_;

  // Verify that UpdateRecord calls come at the recording interval with
  // correct values.
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);

  // Small changes to framerate should not trigger a new record.
  pipeline_framerate_ = kDefaultFps + .5;
  EXPECT_CALL(*interceptor_, StartNewRecord(_, _, _)).Times(0);
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);

  // Small changes in the other direction should also not trigger a new record.
  pipeline_framerate_ = kDefaultFps - .5;
  EXPECT_CALL(*interceptor_, StartNewRecord(_, _, _)).Times(0);
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);

  // Big changes in framerate should trigger a new record.
  pipeline_framerate_ = kDefaultFps * 2;

  // Fast forward by one interval to detect framerate change.
  EXPECT_CALL(*this, GetPipelineStatsCB());
  FastForwardMsec(kRecordingIntervalMs);

  // Stabilize new framerate.
  StabilizeFramerateAndStartNewRecord(kDefaultProfile, kDefaultSize_,
                                      kDefaultFps * 2);

  // Update offsets for new record and verify recording.
  decoded_offset = pipeline_decoded_frames_;
  dropped_offset = pipeline_dropped_frames_;
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);

  // Whacky framerates should be bucketed to a more common nearby value.
  pipeline_framerate_ = 123.4;

  // Fast forward by one interval to detect framerate change.
  EXPECT_CALL(*this, GetPipelineStatsCB());
  FastForwardMsec(kRecordingIntervalMs);

  // Verify new record uses bucketed framerate.
  int bucketed_fps = GetFpsBucket(pipeline_framerate_);
  EXPECT_NE(bucketed_fps, pipeline_framerate_);
  StabilizeFramerateAndStartNewRecord(kDefaultProfile, kDefaultSize_,
                                      bucketed_fps);

  // Update offsets for new record and verify recording.
  decoded_offset = pipeline_decoded_frames_;
  dropped_offset = pipeline_dropped_frames_;
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);
}

TEST_F(MediaCapabilitiesReporterTest, ResolutionBucketing) {
  StartPlayingAndStabilizeFramerate();
  EXPECT_EQ(kDefaultFps, pipeline_framerate_);

  // Framerate is now stable! Recorded stats should be offset by the values
  // last provided to GetPipelineStatsCB.
  uint32_t decoded_offset = pipeline_decoded_frames_;
  uint32_t dropped_offset = pipeline_dropped_frames_;

  // Verify that UpdateRecord calls come at the recording interval with
  // correct values.
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);

  // Small changes to resolution should NOT trigger a new record.
  reporter_->OnNaturalSizeChanged(
      gfx::Size(kDefaultWidth + 2, kDefaultHeight + 2));
  EXPECT_CALL(*interceptor_, StartNewRecord(_, _, _)).Times(0);
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);

  // Big changes in resolution should trigger a new record.
  gfx::Size big_resolution(kDefaultWidth * 2, kDefaultHeight * 2);
  reporter_->OnNaturalSizeChanged(big_resolution);

  // Fast forward by one interval to detect resolution change.
  EXPECT_CALL(*this, GetPipelineStatsCB());
  FastForwardMsec(kRecordingIntervalMs);

  // Stabilize new framerate.
  StabilizeFramerateAndStartNewRecord(kDefaultProfile, big_resolution,
                                      kDefaultFps);

  // Update offsets for new record and verify recording.
  decoded_offset = pipeline_decoded_frames_;
  dropped_offset = pipeline_dropped_frames_;
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);

  // Whacky resolutions should be bucketed into more common nearby dimensions.
  gfx::Size weird_resolution(1234, 5678);
  reporter_->OnNaturalSizeChanged(weird_resolution);

  // Fast forward by one interval to detect resolution change.
  EXPECT_CALL(*this, GetPipelineStatsCB());
  FastForwardMsec(kRecordingIntervalMs);

  // After re-stabilizing framerate, a new record should be started with
  // the bucketed equivalent of the weird resolution.
  gfx::Size bucketed_resolution = GetSizeBucket(weird_resolution);
  EXPECT_NE(bucketed_resolution, weird_resolution);
  StabilizeFramerateAndStartNewRecord(kDefaultProfile, bucketed_resolution,
                                      kDefaultFps);

  // Update offsets for new record and verify recording.
  decoded_offset = pipeline_decoded_frames_;
  dropped_offset = pipeline_dropped_frames_;
  AdvanceTimeAndVerifyRecordUpdate(decoded_offset, dropped_offset);
}

}  // namespace media
