// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/codec/webrtc_video_encoder_gpu.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace remoting {

TEST(WebrtcVideoEncoderGpuTest, BasicTest) {
  std::unique_ptr<WebrtcVideoEncoderGpu> encoder =
      WebrtcVideoEncoderGpu::CreateForH264();
  ASSERT_EQ(nullptr, encoder);
}

}  // namespace remoting
