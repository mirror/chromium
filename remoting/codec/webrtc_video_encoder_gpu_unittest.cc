// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#if defined(OS_WIN)

#include "remoting/codec/webrtc_video_encoder_gpu.h"

#include "base/bind.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "media/gpu/media_foundation_video_encode_accelerator_win.h"
#include "remoting/test/cyclic_frame_generator.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {

class WebrtcVideoEncoderGpuTest : public testing::Test {
 public:
  void OnEncoded(std::unique_ptr<WebrtcVideoEncoder::EncodedFrame> frame) {
    DVLOG(1) << "Frame encoded!";
  }
  void EncodeFrames() {
    const int kTotalFrames = 3;
    for (int i = 0; i < kTotalFrames; i++) {
      std::unique_ptr<webrtc::DesktopFrame> frame =
          frame_generator_->GenerateFrame(nullptr);
      WebrtcVideoEncoder::FrameParams params;
      params.duration = base::TimeDelta::FromSeconds(1);
      encoder_->Encode(std::move(frame), params,
                       base::BindOnce(&WebrtcVideoEncoderGpuTest::OnEncoded,
                                      base::Unretained(this)));
    }
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  scoped_refptr<test::CyclicFrameGenerator> frame_generator_;
  std::unique_ptr<WebrtcVideoEncoderGpu> encoder_;

 private:
  void SetUp() override {
    media::MediaFoundationVideoEncodeAccelerator::PreSandboxInitialization();
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
                   base::BindOnce(&WebrtcVideoEncoderGpuTest::OnEncoded,
                                  base::Unretained(this)));

  base::TimeDelta delay = base::TimeDelta::FromSeconds(5);
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&WebrtcVideoEncoderGpuTest::EncodeFrames,
                 base::Unretained(this)),
      delay);

  base::RunLoop run_loop;
  run_loop.Run();
}

}  // namespace remoting
#endif
