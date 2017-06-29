// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(OS_WIN)

#include "remoting/codec/webrtc_video_encoder_gpu.h"

#include "base/test/scoped_task_environment.h"
#include "base/threading/thread_task_runner_handle.h"
#include "media/gpu/media_foundation_video_encode_accelerator_win.h"
#include "remoting/test/cyclic_frame_generator.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {

class WebrtcVideoEncoderGpuTest : public testing::Test {
 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

 private:
  void SetUp() override {
    media::MediaFoundationVideoEncodeAccelerator::PreSandboxInitialization();
    frame_generator_ = CyclicFrameGenerator::Create();
  };

  std::unique_ptr<CyclicFrameGenerator> frame_generator_;
};

TEST_F(WebrtcVideoEncoderGpuTest, BasicTest) {
  const int kTotalFrames = 100;

  std::unique_ptr<WebrtcVideoEncoderGpu> encoder =
      WebrtcVideoEncoderGpu::CreateForH264();
  ASSERT_NE(nullptr, encoder);

  for (int i = 0; i < kTotalFrames; i++) {
    std::unique_ptr<webrtc::DesktopFrame> frame =
        frame_generator_->GenerateFrame(nullptr);
    encoder->Encode(frame, [](std::unique_ptr<EncodedFrame> frame) {
      DVLOG(1) << "Frame encoded!";
    });
  }
}

}  // namespace remoting

#endif
