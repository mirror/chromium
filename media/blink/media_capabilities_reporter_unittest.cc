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

// #include "base/run_loop.h"

using ::testing::Return;

namespace media {

const VideoCodec kDefaultCodec = kCodecVP9;
const VideoCodecProfile kDefaultProfile = VP9PROFILE_PROFILE0;
const int kDefaultHeight = 480;
const int kDefaultWidth = 640;
// const int kDefaultFps = 30;

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
  MediaCapabilitiesReporterTest() {}

  void SetUp() override {
    task_runner_ = new base::TestMockTimeTaskRunner();
    message_loop_.SetTaskRunner(task_runner_);
  }

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

  MOCK_METHOD0(GetPipelineStatsCB, PipelineStatistics());

 protected:
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
};

TEST_F(MediaCapabilitiesReporterTest, Foo) {
  MakeReporter();

  EXPECT_CALL(*this, GetPipelineStatsCB())
      .WillOnce(Return(MakeStats(10, 0, 30)));
  reporter_->OnPlaying();
  task_runner_->FastForwardBy(base::TimeDelta::FromSeconds(2));
}

}  // namespace media