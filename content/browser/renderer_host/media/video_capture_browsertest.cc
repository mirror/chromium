// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "content/browser/browser_main_loop.h"
#include "content/browser/renderer_host/media/media_stream_manager.h"
#include "content/browser/renderer_host/media/video_capture_manager.h"
#include "content/public/common/content_switches.h"
#include "content/public/test/content_browser_test.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_switches.h"
#include "media/capture/video_capture_types.h"
#include "testing/gmock/include/gmock/gmock.h"

using testing::_;
using testing::AtLeast;
using testing::Invoke;
using testing::InvokeWithoutArgs;
using testing::Values;

namespace content {

class MockVideoCaptureControllerEventHandler
    : public VideoCaptureControllerEventHandler {
 public:
  MOCK_METHOD4(DoOnBufferCreated,
               void(VideoCaptureControllerID id,
                    mojo::ScopedSharedBufferHandle* handle,
                    int length,
                    int buffer_id));
  MOCK_METHOD2(OnBufferDestroyed,
               void(VideoCaptureControllerID, int buffer_id));
  MOCK_METHOD3(OnBufferReady,
               void(VideoCaptureControllerID id,
                    int buffer_id,
                    const media::mojom::VideoFrameInfoPtr& frame_info));
  MOCK_METHOD1(OnStarted, void(VideoCaptureControllerID));
  MOCK_METHOD1(OnEnded, void(VideoCaptureControllerID));
  MOCK_METHOD1(OnError, void(VideoCaptureControllerID));
  MOCK_METHOD1(OnStartedUsingGpuDecode, void(VideoCaptureControllerID));
  MOCK_METHOD1(OnStoppedUsingGpuDecode, void(VideoCaptureControllerID));

  void OnBufferCreated(VideoCaptureControllerID id,
                       mojo::ScopedSharedBufferHandle handle,
                       int length,
                       int buffer_id) override {
    DoOnBufferCreated(id, &handle, length, buffer_id);
  }
};

class MockMediaStreamProviderListener : public MediaStreamProviderListener {
 public:
  MOCK_METHOD2(Opened, void(MediaStreamType, int));
  MOCK_METHOD2(Closed, void(MediaStreamType, int));
  MOCK_METHOD2(Aborted, void(MediaStreamType, int));
};

struct TestParams {
  std::string fake_device_factory_config_string;
  size_t device_index_to_use;
  media::VideoPixelFormat pixel_format_to_use;
  gfx::Size resolution_to_use;
  float frame_rate_to_use;
};

struct FrameInfo {
  gfx::Size size;
  media::VideoPixelFormat pixel_format;
  media::VideoPixelStorage storage_type;
  base::TimeDelta timestamp;
};

class VideoCaptureBrowserTest
    : public ContentBrowserTest,
      public ::testing::WithParamInterface<TestParams> {
 public:
  void SetUpAndStartCaptureDeviceOnIOThread(base::Closure continuation) {
    video_capture_manager_ = media_stream_manager_->video_capture_manager();
    ASSERT_TRUE(video_capture_manager_);
    video_capture_manager_->RegisterListener(&mock_stream_provider_listener_);
    video_capture_manager_->EnumerateDevices(
        base::Bind(&VideoCaptureBrowserTest::OnDeviceDescriptorsReceived,
                   base::Unretained(this), std::move(continuation)));
  }

  void TearDownCaptureDeviceOnIOThread(base::Closure continuation,
                                       bool post_to_end_of_message_queue) {
    // StopCaptureForClient must not be called synchronously from either the
    // |done_cb| passed to StartCaptureForClient() nor any callback made to a
    // VideoCaptureControllerEventHandler. To satisfy this, we have to post our
    // invocation to the end of the IO message queue.
    if (post_to_end_of_message_queue) {
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE,
          base::Bind(&VideoCaptureBrowserTest::TearDownCaptureDeviceOnIOThread,
                     base::Unretained(this), continuation, false));
      return;
    }

    video_capture_manager_->StopCaptureForClient(
        controller_.get(), stub_client_id_, &mock_controller_event_handler_,
        false);

    EXPECT_CALL(mock_stream_provider_listener_, Closed(_, _))
        .WillOnce(InvokeWithoutArgs([continuation]() { continuation.Run(); }));

    video_capture_manager_->Close(session_id_);
  }

 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(
        switches::kUseFakeDeviceForMediaStream,
        GetParam().fake_device_factory_config_string);
    command_line->AppendSwitch(switches::kUseFakeUIForMediaStream);
  }

  // This cannot be part of an override of SetUp(), because at the time when
  // SetUp() is invoked, the BrowserMainLoop does not exist yet.
  void SetUpRequiringBrowserMainLoopOnMainThread() {
    BrowserMainLoop* browser_main_loop = BrowserMainLoop::GetInstance();
    ASSERT_TRUE(browser_main_loop);
    media_stream_manager_ = browser_main_loop->media_stream_manager();
    ASSERT_TRUE(media_stream_manager_);
  }

  void OnDeviceDescriptorsReceived(
      base::Closure continuation,
      const media::VideoCaptureDeviceDescriptors& descriptors) {
    ASSERT_TRUE(GetParam().device_index_to_use < descriptors.size());
    const auto& descriptor = descriptors[GetParam().device_index_to_use];
    MediaStreamDevice media_stream_device(
        MEDIA_DEVICE_VIDEO_CAPTURE, descriptor.device_id,
        descriptor.display_name, descriptor.facing);
    session_id_ = video_capture_manager_->Open(media_stream_device);
    media::VideoCaptureParams capture_params;
    capture_params.requested_format = media::VideoCaptureFormat(
        GetParam().resolution_to_use, GetParam().frame_rate_to_use,
        GetParam().pixel_format_to_use);
    video_capture_manager_->StartCaptureForClient(
        session_id_, capture_params, stub_client_id_,
        &mock_controller_event_handler_,
        base::Bind(&VideoCaptureBrowserTest::OnConnectClientToControllerAnswer,
                   base::Unretained(this), std::move(continuation)));
  }

  void OnConnectClientToControllerAnswer(
      base::Closure continuation,
      const base::WeakPtr<VideoCaptureController>& controller) {
    ASSERT_TRUE(controller.get());
    controller_ = controller;
    if (!continuation)
      return;
    continuation.Run();
  }

 protected:
  MediaStreamManager* media_stream_manager_ = nullptr;
  VideoCaptureManager* video_capture_manager_ = nullptr;
  int session_id_ = 0;
  const VideoCaptureControllerID stub_client_id_ = 123;
  MockMediaStreamProviderListener mock_stream_provider_listener_;
  MockVideoCaptureControllerEventHandler mock_controller_event_handler_;
  base::WeakPtr<VideoCaptureController> controller_;
};

IN_PROC_BROWSER_TEST_P(VideoCaptureBrowserTest, StartAndImmediatelyStop) {
  SetUpRequiringBrowserMainLoopOnMainThread();
  base::RunLoop run_loop;
  auto quit_run_loop_on_current_thread_cb =
      media::BindToCurrentLoop(run_loop.QuitClosure());
  auto after_start_continuation =
      base::Bind(&VideoCaptureBrowserTest::TearDownCaptureDeviceOnIOThread,
                 base::Unretained(this),
                 std::move(quit_run_loop_on_current_thread_cb), true);
  BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&VideoCaptureBrowserTest::SetUpAndStartCaptureDeviceOnIOThread,
                 base::Unretained(this), std::move(after_start_continuation)));
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_P(VideoCaptureBrowserTest,
                       ReceiveFramesFromFakeCaptureDevice) {
  SetUpRequiringBrowserMainLoopOnMainThread();

  std::vector<FrameInfo> received_frame_infos;
  static const size_t kNumFramesToReceive = 3;
  base::RunLoop run_loop;

  auto quit_run_loop_on_current_thread_cb =
      media::BindToCurrentLoop(run_loop.QuitClosure());
  auto finish_test_cb =
      base::Bind(&VideoCaptureBrowserTest::TearDownCaptureDeviceOnIOThread,
                 base::Unretained(this),
                 std::move(quit_run_loop_on_current_thread_cb), true);

  EXPECT_CALL(mock_controller_event_handler_, DoOnBufferCreated(_, _, _, _))
      .Times(AtLeast(1));
  EXPECT_CALL(mock_controller_event_handler_, OnBufferReady(_, _, _))
      .WillRepeatedly(
          Invoke([&received_frame_infos, &finish_test_cb](
                     VideoCaptureControllerID id, int buffer_id,
                     const media::mojom::VideoFrameInfoPtr& frame_info) {
            FrameInfo received_frame_info;
            received_frame_info.pixel_format = frame_info->pixel_format;
            received_frame_info.storage_type = frame_info->storage_type;
            received_frame_info.size = frame_info->coded_size;
            received_frame_info.timestamp = frame_info->timestamp;
            received_frame_infos.emplace_back(received_frame_info);
            if (received_frame_infos.size() >= kNumFramesToReceive) {
              finish_test_cb.Run();
            }
          }));

  base::Closure do_nothing;
  BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&VideoCaptureBrowserTest::SetUpAndStartCaptureDeviceOnIOThread,
                 base::Unretained(this), std::move(do_nothing)));
  run_loop.Run();

  EXPECT_GE(received_frame_infos.size(), kNumFramesToReceive);
  base::TimeDelta previous_timestamp;
  bool first_frame = true;
  for (const auto& frame_info : received_frame_infos) {
    EXPECT_EQ(GetParam().pixel_format_to_use, frame_info.pixel_format);
    EXPECT_EQ(media::PIXEL_STORAGE_CPU, frame_info.storage_type);
    EXPECT_EQ(GetParam().resolution_to_use, frame_info.size);
    // Timestamps are expected to increase
    if (!first_frame)
      EXPECT_GT(frame_info.timestamp, previous_timestamp);
    first_frame = false;
    previous_timestamp = frame_info.timestamp;
  }
}

INSTANTIATE_TEST_CASE_P(
    ,
    VideoCaptureBrowserTest,
    Values(TestParams{"fps=25,device-count=2", 0, media::PIXEL_FORMAT_I420,
                      gfx::Size(1280, 720), 25.0f},
           // The 2nd device outputs Y16
           TestParams{"fps=25,device-count=2", 1, media::PIXEL_FORMAT_Y16,
                      gfx::Size(1280, 720), 25.0f},
           TestParams{"fps=15,device-count=2", 1, media::PIXEL_FORMAT_Y16,
                      gfx::Size(640, 480), 15.0f},
           // The 3rd device outputs MJPEG, which is converted to I420.
           TestParams{"fps=15,device-count=3", 2, media::PIXEL_FORMAT_I420,
                      gfx::Size(640, 480), 25.0f}));

}  // namespace content
