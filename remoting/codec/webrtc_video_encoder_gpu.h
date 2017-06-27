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

class WebrtcVideoEncoderGpu : public WebrtcVideoEncoder,
                              public media::VideoEncodeAccelerator::Client {
 public:
  // TODO(gusss) is this how we want to do it? This is taken from
  // WebrtcVideoEncoderVpx.
  static std::unique_ptr<WebrtcVideoEncoderGpu> CreateForH264();

  ~WebrtcVideoEncoderGpu() override;

  // WebrtcVideoEncoder interface.
  void Encode(const std::unique_ptr<webrtc::DesktopFrame> frame,
              const FrameParams& params,
              WebrtcVideoEncoder::EncodeCallback done) override;
  // Temporary stand-in for the Encode() method above, used to test the
  // encoding pipeline without having yet implemented RGB->YUV conversion.
  void Encode(const scoped_refptr<media::VideoFrame>& frame,
              const FrameParams& params,
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
  explicit WebrtcVideoEncoderGpu(media::VideoCodecProfile codec_profile);

  bool Initialize();

  media::VideoCodecProfile codec_profile_;

  std::unique_ptr<media::VideoEncodeAccelerator> video_encode_accelerator_;

  // Shared memory buffers for input/output with the VEA.
  std::vector<std::unique_ptr<base::SharedMemory>> input_buffers_;
  std::vector<std::unique_ptr<base::SharedMemory>> output_buffers_;

  unsigned int required_input_frame_count_;
  gfx::Size input_coded_size_;
  size_t output_buffer_size_;

  base::flat_map<base::TimeDelta, WebrtcVideoEncoder::EncodeCallback> callbacks_;

  DISALLOW_COPY_AND_ASSIGN(WebrtcVideoEncoderGpu);
};

}  // namespace remoting

#endif  // REMOTING_CODEC_WEBRTC_VIDEO_ENCODER_VEA_H_
