// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/codec/webrtc_video_encoder_gpu.h"

#include "base/memory/ptr_util.h"
#include "gpu/command_buffer/service/gpu_preferences.h"
#include "media/gpu/gpu_video_encode_accelerator_factory.h"

namespace webrtc {
class DesktopFrame;
}

namespace remoting {

WebrtcVideoEncoderGpu::~WebrtcVideoEncoderGpu() {}

// NOTE: see the alternative implementation of Encode() for now. Soon, that
// logic will move in here - but before I've implemented RGB->YUV, I've
// separated it out.
void WebrtcVideoEncoderGpu::Encode(std::unique_ptr<webrtc::DesktopFrame> frame,
                                   const FrameParams& params,
                                   WebrtcVideoEncoder::EncodeCallback done) {
  // TODO(gusss) convert RGB to YUV

  // TODO(gusss) Create VideoFrame w/ timestamp out of DesktopFrame + current
  // timestamp

  // Encode(video_frame, params, done);
}

void WebrtcVideoEncoderGpu::Encode(
    const scoped_refptr<media::VideoFrame>& frame,
    const FrameParams& params,
    WebrtcVideoEncoder::EncodeCallback done) {
  callbacks_[frame->timestamp()] = std::move(done);

  video_encode_accelerator_->Encode(frame, false);
}

void WebrtcVideoEncoderGpu::RequireBitstreamBuffers(
    unsigned int input_count,
    const gfx::Size& input_coded_size,
    size_t output_buffer_size) {
  DVLOG(3) << "WebrtcVideoEncoderGpu::RequireBitstreamBuffers(): "
           << "input_count = " << input_count << ", "
           << "input_coded_size = " << input_coded_size.width() << "x"
           << input_coded_size.height() << ", "
           << "output_buffer_size = " << output_buffer_size;

  required_input_frame_count_ = input_count;
  input_coded_size_ = input_coded_size;
  output_buffer_size_ = output_buffer_size;

  for (unsigned int i = 0; i < input_count; ++i) {
    auto shm = base::MakeUnique<base::SharedMemory>();
    LOG_ASSERT(shm->CreateAndMapAnonymous(output_buffer_size_));
    output_buffers_.push_back(std::move(shm));
  }

  for (size_t i = 0; i < output_buffers_.size(); ++i) {
    video_encode_accelerator_->UseOutputBitstreamBuffer(media::BitstreamBuffer(
        i, output_buffers_[i]->handle(), output_buffers_[i]->mapped_size()));
  }
}

void WebrtcVideoEncoderGpu::BitstreamBufferReady(int32_t bitstream_buffer_id,
                                                 size_t payload_size,
                                                 bool key_frame,
                                                 base::TimeDelta timestamp) {
  DVLOG(3) << "WebrtcVideoEncoderGpu::BitstreamBufferReady(): "
           << "bitstream_buffer_id = " << bitstream_buffer_id << ", "
           << "payload_size = " << payload_size << ", "
           << "key_frame = " << key_frame << ", "
           << "timestamp ms = " << timestamp.InMilliseconds();

  // Get EncodedFrame
  std::unique_ptr<EncodedFrame> encoded_frame =
      base::MakeUnique<EncodedFrame>();
  base::SharedMemory* output_buffer =
      output_buffers_[bitstream_buffer_id].get();
  encoded_frame->data.assign(reinterpret_cast<char*>(output_buffer->memory()),
                             payload_size);

  LOG_ASSERT(callbacks_.find(timestamp) != callbacks_.end())
      << "Callback not found for timestamp " << timestamp;

  WebrtcVideoEncoder::EncodeCallback callback =
      std::move(callbacks_[timestamp]);
  callbacks_.erase(timestamp);

  // TODO(gusss) I saw something similar to this in sergeyu's code - I don't
  // understand what the first std::move does in this case though.
  std::move(callback).Run(std::move(encoded_frame));
}

void WebrtcVideoEncoderGpu::NotifyError(
    media::VideoEncodeAccelerator::Error error) {}

WebrtcVideoEncoderGpu::WebrtcVideoEncoderGpu(media::VideoCodecProfile codec_profile)
    : codec_profile_(codec_profile) {}

// static
std::unique_ptr<WebrtcVideoEncoderGpu> WebrtcVideoEncoderGpu::CreateForH264() {
  // TODO(gusss) picked an arbitrary h264 profile.
  std::unique_ptr<WebrtcVideoEncoderGpu> encoder = base::WrapUnique(
      new WebrtcVideoEncoderGpu(media::VideoCodecProfile::H264PROFILE_MAX));
  if (encoder->Initialize())
    return encoder;
  return nullptr;
}

bool WebrtcVideoEncoderGpu::Initialize() {
  // TODO(gusss) picking arbitrary values for now. Where do these come from in
  // the future?
  // From Sergey: to get input format, we might just delay encoder
  // initialization until we have a number of frames.
  media::VideoPixelFormat input_format =
      media::VideoPixelFormat::PIXEL_FORMAT_YUV420P9;
  gfx::Size input_visible_size = gfx::Size(800, 600);
  uint32_t initial_bitrate = 8 * 1024 * 8;
  gpu::GpuPreferences gpu_preferences;

  video_encode_accelerator_ =
      media::GpuVideoEncodeAcceleratorFactory::CreateVEA(
          input_format, input_visible_size, codec_profile_, initial_bitrate,
          this, gpu_preferences);

  if (!video_encode_accelerator_)
    return false;
  return true;
}

}  // namespace remoting
