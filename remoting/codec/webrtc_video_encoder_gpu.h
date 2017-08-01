// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CODEC_WEBRTC_VIDEO_ENCODER_GPU_H_
#define REMOTING_CODEC_WEBRTC_VIDEO_ENCODER_GPU_H_

#include "media/video/video_encode_accelerator.h"
#include "remoting/codec/webrtc_video_encoder.h"

namespace remoting {

class WebrtcVideoEncoderGpu : public WebrtcVideoEncoder,
                              public media::VideoEncodeAccelerator::Client {
 public:
  static std::unique_ptr<WebrtcVideoEncoderGpu> CreateForH264();

  ~WebrtcVideoEncoderGpu() override;

  // WebrtcVideoEncoder interface.
  void Encode(std::unique_ptr<webrtc::DesktopFrame> frame,
              const FrameParams& params,
              WebrtcVideoEncoder::EncodeCallback done) override;

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
  enum State { UNINITIALIZED, INITIALIZING, INITIALIZED };

  explicit WebrtcVideoEncoderGpu(media::VideoCodecProfile codec_profile);

  bool Initialize(gfx::Size input_visible_size);

  void UseOutputBitstreamBufferId(int32_t bitstream_buffer_id);

  base::OnceClosure pending_encode_;

  State state_;

  std::unique_ptr<media::VideoEncodeAccelerator> video_encode_accelerator_;

  base::TimeDelta previous_timestamp_;

  media::VideoCodecProfile codec_profile_;
  // media::VideoPixelFormat video_pixel_format_;
  scoped_refptr<media::VideoFrame> video_frame_;

  // Shared memory buffers for input/output with the VEA.
  std::vector<uint8_t*> output_buffers_;

  unsigned int required_input_frame_count_;
  gfx::Size input_coded_size_;
  size_t output_buffer_size_;
  gfx::Size input_visible_size_;

  base::flat_map<base::TimeDelta, WebrtcVideoEncoder::EncodeCallback>
      callbacks_;
  // TODO(gusss) this variable should have a clearer name.
  base::flat_map<base::TimeDelta, int> newly_freed_frames_;

  DISALLOW_COPY_AND_ASSIGN(WebrtcVideoEncoderGpu);
};

}  // namespace remoting

#endif  // REMOTING_CODEC_WEBRTC_VIDEO_ENCODER_GPU_H_
