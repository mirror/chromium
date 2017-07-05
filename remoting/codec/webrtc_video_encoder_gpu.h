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
  enum State { UNINITIALIZED = 0, INITIALIZING, INITIALIZED };
  State state_;

  explicit WebrtcVideoEncoderGpu(media::VideoPixelFormat pixel_format,
                                 media::VideoCodecProfile codec_profile);

  // Get the index into |frames_| of a free VideoFrame. The frame index is
  // removed from free_frame_indices_. If there isn't a free frame, allocate a
  // new frame and return its index.
  int GetFreeVideoFrameIndex();

  bool Initialize();

  void UseOutputBitstreamBufferId(int id);

  base::TimeDelta last_timestamp_;

  // webrtc::DesktopSize desktop_size_;
  gfx::Size input_visible_size_;

  media::VideoPixelFormat pixel_format_;
  media::VideoCodecProfile codec_profile_;

  std::unique_ptr<media::VideoEncodeAccelerator> video_encode_accelerator_;

  // VideoFrames are used as input into the VideoEncodeAccelerator. Currently,
  // frames are allocated as they are needed. Free frames have their indices in
  // free_frame_indices_, and are pushed/popped in stack fashion.
  // TODO(gusss) would we rather have a constant number of frames, and force
  // Encode() requests to wait for a free frame?
  std::vector<int> free_frame_indices_;
  std::vector<scoped_refptr<media::VideoFrame>> frames_;

  // media::VideoPixelFormat video_pixel_format_;

  // Shared memory buffers for input/output with the VEA.
  std::vector<std::unique_ptr<base::SharedMemory>> input_buffers_;
  std::vector<std::unique_ptr<base::SharedMemory>> output_buffers_;

  unsigned int required_input_frame_count_;
  gfx::Size input_coded_size_;
  size_t output_buffer_size_;

  base::flat_map<base::TimeDelta, WebrtcVideoEncoder::EncodeCallback>
      callbacks_;
  // TODO(gusss) naming
  base::flat_map<base::TimeDelta, int> newly_freed_frames_;

  DISALLOW_COPY_AND_ASSIGN(WebrtcVideoEncoderGpu);
};

}  // namespace remoting

#endif  // REMOTING_CODEC_WEBRTC_VIDEO_ENCODER_VEA_H_
