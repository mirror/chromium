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

  bool ShouldBeReporting() { return reporter_->ShouldBeReporting(); }

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

    // Now advance time just enough for framerate to stabilize. We only need
    // iterations = (kRequiredStableSamples - 1) because the first stats
    // callback above counts as the first stable framerate sample.
    double frame_duration_ms =
        (1.0 / pipeline_framerate_) * kMillisecondsPerSecond;
    for (int i = 0; i < kRequiredStableFpsSamples - 1; i++) {
      // The final iteration stabilizes framerate and starts a new record.
      if (i == kRequiredStableFpsSamples - 2) {
        EXPECT_CALL(*interceptor_, StartNewRecord(kDefaultProfile,
                                                  kDefaultSize_, kDefaultFps));
      }

      // The timer runs at 3x the frame duration when detecting framerate to
      // quickly stabilize.
      EXPECT_CALL(*this, GetPipelineStatsCB());
      FastForwardMsec(frame_duration_ms * 3);
    }
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

  // With framerate stable, UpdateStats calls can begin. But the stats will be
  // offset from the value observed at the time of stabilization.
  uint32_t decoded_offset = pipeline_decoded_frames_;
  uint32_t dropped_offset = pipeline_dropped_frames_;

  // Verify that UpdateRecord calls come at the recording interval with correct
  // values.
  EXPECT_CALL(*this, GetPipelineStatsCB());
  PipelineStatistics next_stats = PeekNextDecodeStats();
  EXPECT_CALL(*interceptor_,
              UpdateRecord(next_stats.video_frames_decoded - decoded_offset,
                           next_stats.video_frames_dropped - dropped_offset));
  FastForwardMsec(kRecordingIntervalMs);

  // Once more for good measure. Save previous counts to verify they advanced.
  uint32_t prev_decoded_frames = pipeline_decoded_frames_;
  uint32_t prev_dropped_frames = pipeline_dropped_frames_;
  EXPECT_CALL(*this, GetPipelineStatsCB());
  next_stats = PeekNextDecodeStats();
  EXPECT_CALL(*interceptor_,
              UpdateRecord(next_stats.video_frames_decoded - decoded_offset,
                           next_stats.video_frames_dropped - dropped_offset));
  FastForwardMsec(kRecordingIntervalMs);

  EXPECT_GT(pipeline_decoded_frames_, prev_decoded_frames);
  EXPECT_GT(pipeline_dropped_frames_, prev_dropped_frames);
}

TEST_F(MediaCapabilitiesReporterTest, RecordingStopsWhenPaused) {
  StartPlayingAndStabilizeFramerate();

  // With framerate stable, UpdateStats calls can begin. But the stats will be
  // offset from the value observed at the time of stabilization.
  uint32_t decoded_offset = pipeline_decoded_frames_;
  uint32_t dropped_offset = pipeline_dropped_frames_;

  // Verify that UpdateRecord calls come at the recording interval with correct
  // values.
  EXPECT_CALL(*this, GetPipelineStatsCB());
  PipelineStatistics next_stats = PeekNextDecodeStats();
  EXPECT_CALL(*interceptor_,
              UpdateRecord(next_stats.video_frames_decoded - decoded_offset,
                           next_stats.video_frames_dropped - dropped_offset));
  FastForwardMsec(kRecordingIntervalMs);

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
  EXPECT_CALL(*this, GetPipelineStatsCB());
  next_stats = PeekNextDecodeStats();
  EXPECT_CALL(*interceptor_,
              UpdateRecord(next_stats.video_frames_decoded - decoded_offset,
                           next_stats.video_frames_dropped - dropped_offset));
  FastForwardMsec(kRecordingIntervalMs);
}

TEST_F(MediaCapabilitiesReporterTest, RecordingStopsWhenHidden) {
  StartPlayingAndStabilizeFramerate();

  // With framerate stable, UpdateStats calls can begin. But the stats will be
  // offset from the value observed at the time of stabilization.
  uint32_t decoded_offset = pipeline_decoded_frames_;
  uint32_t dropped_offset = pipeline_dropped_frames_;

  // Verify that UpdateRecord calls come at the recording interval with correct
  // values.
  EXPECT_CALL(*this, GetPipelineStatsCB());
  PipelineStatistics next_stats = PeekNextDecodeStats();
  EXPECT_CALL(*interceptor_,
              UpdateRecord(next_stats.video_frames_decoded - decoded_offset,
                           next_stats.video_frames_dropped - dropped_offset));
  FastForwardMsec(kRecordingIntervalMs);

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
  EXPECT_CALL(*this, GetPipelineStatsCB());
  next_stats = PeekNextDecodeStats();
  EXPECT_CALL(*interceptor_,
              UpdateRecord(next_stats.video_frames_decoded - decoded_offset,
                           next_stats.video_frames_dropped - dropped_offset));
  FastForwardMsec(kRecordingIntervalMs);
}

TEST_F(MediaCapabilitiesReporterTest, RecordingStopsWhenNoDecodeProgress) {
  StartPlayingAndStabilizeFramerate();

  // With framerate stable, UpdateStats calls can begin. But the stats will be
  // offset from the value observed at the time of stabilization.
  uint32_t decoded_offset = pipeline_decoded_frames_;
  uint32_t dropped_offset = pipeline_dropped_frames_;

  // Verify that UpdateRecord calls come at the recording interval with correct
  // values.
  EXPECT_CALL(*this, GetPipelineStatsCB());
  PipelineStatistics next_stats = PeekNextDecodeStats();
  EXPECT_CALL(*interceptor_,
              UpdateRecord(next_stats.video_frames_decoded - decoded_offset,
                           next_stats.video_frames_dropped - dropped_offset));
  FastForwardMsec(kRecordingIntervalMs);

  // Freeze decode stats at current values, simulating network underflow.
  ON_CALL(*this, GetPipelineStatsCB())
      .WillByDefault(
          Return(MakeStats(pipeline_decoded_frames_, pipeline_dropped_frames_,
                           pipeline_framerate_)));

  // Verify record updates stop while decode is not progressing.
  EXPECT_CALL(*this, GetPipelineStatsCB());
  EXPECT_CALL(*interceptor_, UpdateRecord(_, _)).Times(0);
  FastForwardMsec(kRecordingIntervalMs);

  // FastForward a bit more to be sure we still don't call UpdateRecord.
  EXPECT_CALL(*this, GetPipelineStatsCB()).Times(3);
  FastForwardMsec(kRecordingIntervalMs * 3);

  // Resume progressing decode!
  ON_CALL(*this, GetPipelineStatsCB())
      .WillByDefault(Invoke(
          this, &MediaCapabilitiesReporterTest::MakeAdvancingDecodeStats));

  // Verify record updates resume.
  EXPECT_CALL(*this, GetPipelineStatsCB());
  next_stats = PeekNextDecodeStats();
  EXPECT_CALL(*interceptor_,
              UpdateRecord(next_stats.video_frames_decoded - decoded_offset,
                           next_stats.video_frames_dropped - dropped_offset));
  FastForwardMsec(kRecordingIntervalMs);
}

TEST_F(MediaCapabilitiesReporterTest, NewRecordStartsForSizeChange) {
  StartPlayingAndStabilizeFramerate();

  // With framerate stable, UpdateStats calls can begin. But the stats will be
  // offset from the value observed at the time of stabilization.
  uint32_t decoded_offset = pipeline_decoded_frames_;
  uint32_t dropped_offset = pipeline_dropped_frames_;

  // Verify that UpdateRecord calls come at the recording interval with correct
  // values.
  EXPECT_CALL(*this, GetPipelineStatsCB());
  PipelineStatistics next_stats = PeekNextDecodeStats();
  EXPECT_CALL(*interceptor_,
              UpdateRecord(next_stats.video_frames_decoded - decoded_offset,
                           next_stats.video_frames_dropped - dropped_offset));
  FastForwardMsec(kRecordingIntervalMs);

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
  double frame_duration_ms =
      (1.0 / pipeline_framerate_) * kMillisecondsPerSecond;
  for (int i = 0; i < kRequiredStableFpsSamples - 1; i++) {
    if (i == kRequiredStableFpsSamples - 2) {
      // Verify new record uses the new size.
      EXPECT_CALL(*interceptor_,
                  StartNewRecord(kDefaultProfile, size_720p, kDefaultFps));
    }
    // The timer runs at 3x the frame duration when detecting framerate to
    // quickly stabilize.
    EXPECT_CALL(*this, GetPipelineStatsCB());
    FastForwardMsec(frame_duration_ms * 3);
  }

  // Offsets should be adjusted so the new record starts at zero.
  decoded_offset = pipeline_decoded_frames_;
  dropped_offset = pipeline_dropped_frames_;

  // Stats callbacks and record updates should proceed as usual.
  EXPECT_CALL(*this, GetPipelineStatsCB());
  next_stats = PeekNextDecodeStats();
  EXPECT_CALL(*interceptor_,
              UpdateRecord(next_stats.video_frames_decoded - decoded_offset,
                           next_stats.video_frames_dropped - dropped_offset));
  FastForwardMsec(kRecordingIntervalMs);
}

// TESTS TO WRITE
// 6. New record started for config change
// 7. New record started for fps change
// 8. FPS stabilization fails -> timer stops until config change
// 9. FPS stabilization tiny windows -> timer stops until config change
// 10. Recording stops when hidden, config/size change still processed though
// 11. fps stabiliaztion Timer slows if no decode progress.

}  // namespace media