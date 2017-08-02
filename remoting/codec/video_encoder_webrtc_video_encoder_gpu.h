// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CODEC_VIDEO_ENCODER_WEBRTC_VIDEO_ENCODER_GPU_H_
#define REMOTING_CODEC_VIDEO_ENCODER_WEBRTC_VIDEO_ENCODER_GPU_H_

#include "base/macros.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "remoting/codec/video_encoder.h"
#include "remoting/codec/webrtc_video_encoder.h"
#include "remoting/codec/webrtc_video_encoder_gpu.h"

namespace base {
class SequencedTaskRunner;
}

namespace remoting {

// VideoEncoderWebrtcVideoEncoderGpu implements a VideoEncoder which wraps over
// a WebrtcVideoEncoderGpu. Note that calls to Encode will encode the given
// frame, but will return a blank frame. This is because this class is used
// solely in the it2me_standalone_host profiler. It allows us to use said
// profiler to time the hardware encoder.
class VideoEncoderWebrtcVideoEncoderGpu : public VideoEncoder {
 public:
  VideoEncoderWebrtcVideoEncoderGpu();
  ~VideoEncoderWebrtcVideoEncoderGpu() override;

  // VideoEncoder interface.
  std::unique_ptr<VideoPacket> Encode(
      const webrtc::DesktopFrame& frame) override;

  void Callback(base::WaitableEvent*,
                std::unique_ptr<WebrtcVideoEncoder::EncodedFrame>);
  void DoEncode(const webrtc::DesktopFrame& frame, base::WaitableEvent* event);

 private:
  std::unique_ptr<WebrtcVideoEncoderGpu> encoder_;
  std::unique_ptr<WebrtcVideoEncoder::EncodedFrame> encoded_frame_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::Thread gpu_encode_thread_;

  DISALLOW_COPY_AND_ASSIGN(VideoEncoderWebrtcVideoEncoderGpu);
};

}  // namespace remoting

#endif  // REMOTING_CODEC_VIDEO_ENCODER_WEBRTC_VIDEO_ENCODER_GPU_H_
