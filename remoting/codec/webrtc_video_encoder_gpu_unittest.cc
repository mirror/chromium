// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include "remoting/codec/webrtc_video_encoder_gpu.h"

#include "base/synchronization/waitable_event.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#if defined(OS_WIN)
#include "media/gpu/media_foundation_video_encode_accelerator_win.h"
#endif
#include "remoting/test/cyclic_frame_generator.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {

class WebrtcVideoEncoderGpuTest : public testing::Test {
 public:
  void Callback(base::WaitableEvent* event,
                std::unique_ptr<WebrtcVideoEncoder::EncodedFrame> frame) {
    DVLOG(3) << "Frame encoded!";
    if (event)
      event->Signal();
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  scoped_refptr<test::CyclicFrameGenerator> frame_generator_;
  std::unique_ptr<WebrtcVideoEncoderGpu> encoder_;

 private:
  void SetUp() override {

#if defined(OS_WIN)
    media::MediaFoundationVideoEncodeAccelerator::PreSandboxInitialization();
#endif
    frame_generator_ = test::CyclicFrameGenerator::Create();
  };
};

TEST_F(WebrtcVideoEncoderGpuTest, BasicTest) {
  encoder_ = WebrtcVideoEncoderGpu::CreateForH264();
  ASSERT_NE(nullptr, encoder_);

  std::unique_ptr<webrtc::DesktopFrame> frame =
      frame_generator_->GenerateFrame(nullptr);
  WebrtcVideoEncoder::FrameParams params;
  params.duration = base::TimeDelta::FromSeconds(1);
  encoder_->Encode(std::move(frame), params,
                   base::Bind(&WebrtcVideoEncoderGpuTest::Callback,
                              base::Unretained(this), nullptr));

  base::RunLoop run_loop;
  base::TimeDelta duration = base::TimeDelta::FromSeconds(2);
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(), duration);

  run_loop.Run();

  frame = frame_generator_->GenerateFrame(nullptr);
  params.duration = base::TimeDelta::FromSeconds(1);
  encoder_->Encode(std::move(frame), params,
                   base::Bind(&WebrtcVideoEncoderGpuTest::Callback,
                              base::Unretained(this), nullptr));
}

}  // namespace remoting
