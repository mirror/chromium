// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/media_capture_from_element/canvas_capture_handler.h"

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "cc/test/test_context_provider.h"
#include "cc/test/test_in_process_context_provider.h"
#include "content/child/child_process.h"
#include "content/renderer/media/media_stream_video_capturer_source.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "media/base/limits.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/WebGraphicsContext3DProvider.h"
#include "third_party/WebKit/public/platform/WebMediaStreamSource.h"
#include "third_party/WebKit/public/platform/WebMediaStreamTrack.h"
#include "third_party/WebKit/public/platform/WebSize.h"
#include "third_party/WebKit/public/web/WebHeap.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkRefCnt.h"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Mock;
using ::testing::SaveArg;
using ::testing::Test;
using ::testing::TestWithParam;

namespace content {

namespace {

static const int kTestCanvasCaptureWidth = 320;
static const int kTestCanvasCaptureHeight = 240;
static const double kTestCanvasCaptureFramesPerSecond = 55.5;

static const int kTestCanvasCaptureFrameEvenSize = 2;
static const int kTestCanvasCaptureFrameOddSize = 3;
static const int kTestCanvasCaptureFrameColorErrorTolerance = 2;
static const int kTestAlphaValue = 175;

ACTION_P(RunClosure, closure) {
  closure.Run();
}

}  // namespace

class FakeWebGraphicsContext3DProvider
    : public blink::WebGraphicsContext3DProvider {
 public:
  bool BindToCurrentThread() override { return true; }
  gpu::gles2::GLES2Interface* ContextGL() override { return nullptr; }
  GrContext* GetGrContext() override { return nullptr; }
  const gpu::Capabilities& GetCapabilities() const override { return caps; }
  const gpu::GpuFeatureInfo& GetGpuFeatureInfo() const override { return info; }
  bool IsSoftwareRendering() const override { return true; }
  void SetLostContextCallback(const base::Closure&) override {}
  void SetErrorMessageCallback(
      const base::Callback<void(const char*, int32_t)>&) override {}
  void SignalQuery(uint32_t, const base::Closure&) override {}
  gpu::Capabilities caps;
  gpu::GpuFeatureInfo info;
};

class CanvasCaptureHandlerTest
    : public TestWithParam<testing::tuple<bool /* opaque */,
                                          int /* width */,
                                          int /* height */>> {
 public:
  CanvasCaptureHandlerTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}

  void SetUp() override {
    context_provider_ = new cc::TestInProcessContextProvider(nullptr);
    context_provider_->BindToCurrentThread();
    web_context_provider_.reset(new FakeWebGraphicsContext3DProvider());
    canvas_capture_handler_ = CanvasCaptureHandler::CreateCanvasCaptureHandler(
        blink::WebSize(kTestCanvasCaptureWidth, kTestCanvasCaptureHeight),
        kTestCanvasCaptureFramesPerSecond,
        scoped_task_environment_.GetMainThreadTaskRunner(), &track_);
  }

  void TearDown() override {
    track_.Reset();
    blink::WebHeap::CollectAllGarbageForTesting();
    canvas_capture_handler_.reset();

    // Let the message loop run to finish destroying the capturer.
    base::RunLoop().RunUntilIdle();
  }

  // Necessary callbacks and MOCK_METHODS for VideoCapturerSource.
  MOCK_METHOD2(DoOnDeliverFrame,
               void(const scoped_refptr<media::VideoFrame>&, base::TimeTicks));
  void OnDeliverFrame(const scoped_refptr<media::VideoFrame>& video_frame,
                      base::TimeTicks estimated_capture_time) {
    DoOnDeliverFrame(video_frame, estimated_capture_time);
  }

  MOCK_METHOD1(DoOnRunning, void(bool));
  void OnRunning(bool state) { DoOnRunning(state); }

  // Verify returned frames.
  sk_sp<SkImage> GenerateTestImage(bool opaque,
                                   int width,
                                   int height,
                                   bool texture_backed) {
    SkBitmap testBitmap;
    testBitmap.allocN32Pixels(width, height, opaque);
    testBitmap.eraseARGB(kTestAlphaValue, 30, 60, 200);
    sk_sp<SkImage> bitmap_image = SkImage::MakeFromBitmap(testBitmap);
    if (!texture_backed)
      return bitmap_image;

    sk_sp<SkImage> tex_image =
        bitmap_image->makeTextureImage(context_provider_->GrContext(), nullptr);
    return tex_image;
  }

  void OnVerifyDeliveredFrame(
      bool opaque,
      int expected_width,
      int expected_height,
      const scoped_refptr<media::VideoFrame>& video_frame,
      base::TimeTicks estimated_capture_time) {
    if (opaque)
      EXPECT_EQ(media::PIXEL_FORMAT_I420, video_frame->format());
    else
      EXPECT_EQ(media::PIXEL_FORMAT_YV12A, video_frame->format());

    EXPECT_EQ(video_frame->timestamp().InMilliseconds(),
              (estimated_capture_time - base::TimeTicks()).InMilliseconds());
    const gfx::Size& size = video_frame->visible_rect().size();
    EXPECT_EQ(expected_width, size.width());
    EXPECT_EQ(expected_height, size.height());
    const uint8_t* y_plane =
        video_frame->visible_data(media::VideoFrame::kYPlane);
    EXPECT_NEAR(74, y_plane[0], kTestCanvasCaptureFrameColorErrorTolerance);
    const uint8_t* u_plane =
        video_frame->visible_data(media::VideoFrame::kUPlane);
    EXPECT_NEAR(193, u_plane[0], kTestCanvasCaptureFrameColorErrorTolerance);
    const uint8_t* v_plane =
        video_frame->visible_data(media::VideoFrame::kVPlane);
    EXPECT_NEAR(105, v_plane[0], kTestCanvasCaptureFrameColorErrorTolerance);
    if (!opaque) {
      const uint8_t* a_plane =
          video_frame->visible_data(media::VideoFrame::kAPlane);
      EXPECT_EQ(kTestAlphaValue, a_plane[0]);
    }

    OnDeliverFrame(video_frame, estimated_capture_time);
  }

  blink::WebMediaStreamTrack track_;
  scoped_refptr<cc::TestInProcessContextProvider> context_provider_;
  std::unique_ptr<FakeWebGraphicsContext3DProvider> web_context_provider_;
  // The Class under test. Needs to be scoped_ptr to force its destruction.
  std::unique_ptr<CanvasCaptureHandler> canvas_capture_handler_;

 protected:
  media::VideoCapturerSource* GetVideoCapturerSource(
      MediaStreamVideoCapturerSource* ms_source) {
    return ms_source->source_.get();
  }

  // A ChildProcess is needed to fool the Tracks and Sources believing they are
  // on the right threads. A ScopedTaskEnvironment must be instantiated before
  // ChildProcess to prevent it from leaking a TaskScheduler.
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  ChildProcess child_process_;

 private:
  DISALLOW_COPY_AND_ASSIGN(CanvasCaptureHandlerTest);
};

// Checks that the initialization-destruction sequence works fine.
TEST_F(CanvasCaptureHandlerTest, ConstructAndDestruct) {
  EXPECT_TRUE(canvas_capture_handler_->NeedsNewFrame());
  base::RunLoop().RunUntilIdle();
}

// Checks that the destruction sequence works fine.
TEST_F(CanvasCaptureHandlerTest, DestructTrack) {
  EXPECT_TRUE(canvas_capture_handler_->NeedsNewFrame());
  track_.Reset();
  base::RunLoop().RunUntilIdle();
}

// Checks that the destruction sequence works fine.
TEST_F(CanvasCaptureHandlerTest, DestructHandler) {
  EXPECT_TRUE(canvas_capture_handler_->NeedsNewFrame());
  canvas_capture_handler_.reset();
  base::RunLoop().RunUntilIdle();
}

// Checks that VideoCapturerSource call sequence works fine.
TEST_P(CanvasCaptureHandlerTest, GetFormatsStartAndStop) {
  InSequence s;
  const blink::WebMediaStreamSource& web_media_stream_source = track_.Source();
  EXPECT_FALSE(web_media_stream_source.IsNull());
  MediaStreamVideoCapturerSource* const ms_source =
      static_cast<MediaStreamVideoCapturerSource*>(
          web_media_stream_source.GetExtraData());
  EXPECT_TRUE(ms_source != nullptr);
  media::VideoCapturerSource* source = GetVideoCapturerSource(ms_source);
  EXPECT_TRUE(source != nullptr);

  media::VideoCaptureFormats formats = source->GetPreferredFormats();
  ASSERT_EQ(2u, formats.size());
  EXPECT_EQ(kTestCanvasCaptureWidth, formats[0].frame_size.width());
  EXPECT_EQ(kTestCanvasCaptureHeight, formats[0].frame_size.height());
  media::VideoCaptureParams params;
  params.requested_format = formats[0];

  base::RunLoop run_loop;
  base::Closure quit_closure = run_loop.QuitClosure();
  EXPECT_CALL(*this, DoOnRunning(true)).Times(1);
  EXPECT_CALL(*this, DoOnDeliverFrame(_, _))
      .Times(1)
      .WillOnce(RunClosure(quit_closure));
  source->StartCapture(
      params,
      base::Bind(&CanvasCaptureHandlerTest::OnDeliverFrame,
                 base::Unretained(this)),
      base::Bind(&CanvasCaptureHandlerTest::OnRunning, base::Unretained(this)));
  canvas_capture_handler_->SendNewFrame(
      GenerateTestImage(testing::get<0>(GetParam()),
                        testing::get<1>(GetParam()),
                        testing::get<2>(GetParam()), false),
      nullptr);
  run_loop.Run();

  source->StopCapture();
}

// Verifies that SkImage is processed and produces VideoFrame as expected.
TEST_P(CanvasCaptureHandlerTest, VerifyFrame) {
  const bool opaque_frame = testing::get<0>(GetParam());
  const bool width = testing::get<1>(GetParam());
  const bool height = testing::get<2>(GetParam());
  InSequence s;
  media::VideoCapturerSource* const source =
      GetVideoCapturerSource(static_cast<MediaStreamVideoCapturerSource*>(
          track_.Source().GetExtraData()));
  EXPECT_TRUE(source != nullptr);

  base::RunLoop run_loop;
  base::Closure quit_closure = run_loop.QuitClosure();
  EXPECT_CALL(*this, DoOnRunning(true)).Times(1);
  EXPECT_CALL(*this, DoOnDeliverFrame(_, _))
      .Times(1)
      .WillOnce(RunClosure(quit_closure));
  media::VideoCaptureParams params;
  source->StartCapture(
      params,
      base::Bind(&CanvasCaptureHandlerTest::OnVerifyDeliveredFrame,
                 base::Unretained(this), opaque_frame, width, height),
      base::Bind(&CanvasCaptureHandlerTest::OnRunning, base::Unretained(this)));
  canvas_capture_handler_->SendNewFrame(
      GenerateTestImage(opaque_frame, width, height, false), nullptr);
  run_loop.RunUntilIdle();
}

// Checks that needsNewFrame() works as expected.
TEST_F(CanvasCaptureHandlerTest, CheckNeedsNewFrame) {
  InSequence s;
  media::VideoCapturerSource* source =
      GetVideoCapturerSource(static_cast<MediaStreamVideoCapturerSource*>(
          track_.Source().GetExtraData()));
  EXPECT_TRUE(source != nullptr);
  EXPECT_TRUE(canvas_capture_handler_->NeedsNewFrame());
  source->StopCapture();
  EXPECT_FALSE(canvas_capture_handler_->NeedsNewFrame());
}

// Verifies that we receive a callback in case of a context loss and
// |gl_helper_| is reset.
TEST_F(CanvasCaptureHandlerTest, HandlesContextLoss) {
  InSequence s;
  media::VideoCapturerSource* const source =
      GetVideoCapturerSource(static_cast<MediaStreamVideoCapturerSource*>(
          track_.Source().GetExtraData()));
  EXPECT_TRUE(source != nullptr);
  EXPECT_CALL(*this, DoOnRunning(true));
  source->StartCapture(
      media::VideoCaptureParams(),
      base::Bind(&CanvasCaptureHandlerTest::OnDeliverFrame,
                 base::Unretained(this)),
      base::Bind(&CanvasCaptureHandlerTest::OnRunning, base::Unretained(this)));

  // Use cc::TestContextProvider as it has SetLostContextCallback()
  // implementation.
  scoped_refptr<cc::TestContextProvider> test_provider =
      cc::TestContextProvider::Create();
  test_provider->BindToCurrentThread();
  test_provider->AddObserver(canvas_capture_handler_.get());

  // Send a frame and verify |gl_helper_| is set.
  sk_sp<SkImage> image =
      GenerateTestImage(true, kTestCanvasCaptureFrameEvenSize,
                        kTestCanvasCaptureFrameEvenSize, true);
  canvas_capture_handler_->SendNewFrameForTesting(
      image, web_context_provider_.get(), test_provider.get());
  EXPECT_TRUE(canvas_capture_handler_->gl_helper_ != nullptr);

  // Force a context loss and check |gl_helper_| is reset.
  base::RunLoop run_loop;
  test_provider->ContextGL()->LoseContextCHROMIUM(
      GL_GUILTY_CONTEXT_RESET_ARB, GL_INNOCENT_CONTEXT_RESET_ARB);
  test_provider->ContextGL()->Flush();
  run_loop.RunUntilIdle();
  EXPECT_TRUE(canvas_capture_handler_->gl_helper_ == nullptr);

  test_provider->RemoveObserver(canvas_capture_handler_.get());
}

INSTANTIATE_TEST_CASE_P(
    ,
    CanvasCaptureHandlerTest,
    ::testing::Combine(::testing::Bool(),
                       ::testing::Values(kTestCanvasCaptureFrameEvenSize,
                                         kTestCanvasCaptureFrameOddSize),
                       ::testing::Values(kTestCanvasCaptureFrameEvenSize,
                                         kTestCanvasCaptureFrameOddSize)));

}  // namespace content
