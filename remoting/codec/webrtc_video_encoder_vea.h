// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CODEC_WEBRTC_VIDEO_ENCODER_VEA_H_
#define REMOTING_CODEC_WEBRTC_VIDEO_ENCODER_VEA_H_

#include <map>
#include "media/video/video_encode_accelerator.h"
#include "remoting/codec/webrtc_video_encoder.h"

namespace base {
class SharedMemory;
}

namespace remoting {

class WebrtcVideoEncoderVea : public WebrtcVideoEncoder,
                              public media::VideoEncodeAccelerator::Client {
 public:
  static std::unique_ptr<WebrtcVideoEncoderVea> CreateForH264();

  ~WebrtcVideoEncoderVea() override;

  // WebrtcVideoEncoder interface.
  void Encode(const std::unique_ptr<webrtc::DesktopFrame> frame,
              const FrameParams& params,
              WebrtcVideoEncoder::EncodeCallback done) override;
  void Encode(const scoped_refptr<media::VideoFrame>& frame,
              WebrtcVideoEncoder::EncodeCallback done);

  // VideoEncodeAccelerator::Client interface.
  void RequireBitstreamBuffers(unsigned int input_count,
                               const gfx::Size& input_coded_size,
                               size_t output_buffer_size) override;
  void BitstreamBufferReady(int32_t bitstream_buffer_id,
                            size_t payload_size,
                            bool key_frame,
                            base::TimeDelta timestamp) override;
  void NotifyError(media::VideoEncodeAccelerator::Error error) override;

 private:
  enum Codec { H264 = 0 };

  explicit WebrtcVideoEncoderVea(Codec codec);

  bool Initialize();

  Codec codec_;

  std::unique_ptr<media::VideoEncodeAccelerator> video_encode_accelerator_;

  // Shared memory buffers for input/output with the VEA.
  std::vector<std::unique_ptr<base::SharedMemory>> input_buffers_;
  std::vector<std::unique_ptr<base::SharedMemory>> output_buffers_;

  unsigned int required_input_frame_count_;
  gfx::Size input_coded_size_;
  size_t output_buffer_size_;

  std::map<base::TimeDelta, WebrtcVideoEncoder::EncodeCallback> callbacks_;
};

}  // namespace remoting

#endif  // REMOTING_CODEC_WEBRTC_VIDEO_ENCODER_VEA_H_
