// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/codec/webrtc_video_encoder_vea.h"
#include "base/memory/ptr_util.h"
#include "gpu/command_buffer/service/gpu_preferences.h"
#include "media/gpu/gpu_video_encode_accelerator_factory.h"

namespace webrtc {
class DesktopFrame;
}

namespace remoting {

WebrtcVideoEncoderVea::~WebrtcVideoEncoderVea() {}

void WebrtcVideoEncoderVea::Encode(std::unique_ptr<webrtc::DesktopFrame> frame,
                                   const FrameParams& params,
                                   WebrtcVideoEncoder::EncodeCallback done) {
  std::unique_ptr<WebrtcVideoEncoder::EncodedFrame> encoded_frame(
      new WebrtcVideoEncoder::EncodedFrame());

  // TODO Create Video Frame with timestamp out of DesktopFrame
}

void WebrtcVideoEncoderVea::Encode(
    const scoped_refptr<media::VideoFrame>& frame,
    WebrtcVideoEncoder::EncodeCallback done) {
  callbacks_[frame->timestamp()] = std::move(done);

  video_encode_accelerator_->Encode(frame, false);
}

void WebrtcVideoEncoderVea::RequireBitstreamBuffers(
    unsigned int input_count,
    const gfx::Size& input_coded_size,
    size_t output_buffer_size) {
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

void WebrtcVideoEncoderVea::BitstreamBufferReady(int32_t bitstream_buffer_id,
                                                 size_t payload_size,
                                                 bool key_frame,
                                                 base::TimeDelta timestamp) {
  DVLOG(3) << "WebrtcVideoEncoderVea::BitstreamBufferReady(): "
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

  if (callbacks_.find(timestamp) == callbacks_.end()) {
    // error case
    LOG(FATAL) << "Callback not found for timestamp " << timestamp;
    return;
  }

  WebrtcVideoEncoder::EncodeCallback callback =
      std::move(callbacks_[timestamp]);
  callbacks_.erase(timestamp);

  std::move(callback).Run(std::move(encoded_frame));
}

void WebrtcVideoEncoderVea::NotifyError(
    media::VideoEncodeAccelerator::Error error) {}

WebrtcVideoEncoderVea::WebrtcVideoEncoderVea(WebrtcVideoEncoderVea::Codec codec)
    : codec_(codec) {}

// static
std::unique_ptr<WebrtcVideoEncoderVea> WebrtcVideoEncoderVea::CreateForH264() {
  std::unique_ptr<WebrtcVideoEncoderVea> encoder = base::WrapUnique(
      new WebrtcVideoEncoderVea(WebrtcVideoEncoderVea::Codec::H264));
  if (encoder->Initialize())
    return encoder;
  return nullptr;
};

bool WebrtcVideoEncoderVea::Initialize() {
  media::VideoPixelFormat input_format =
      media::VideoPixelFormat::PIXEL_FORMAT_YUV420P9;
  gfx::Size input_visible_size = gfx::Size(800, 600);

  media::VideoCodecProfile output_profile;
  switch (codec_) {
    case Codec::H264:
      output_profile = media::H264PROFILE_MAX;
      break;
    default:
      return false;
  }

  uint32_t initial_bitrate = 8 * 1024 * 8;
  gpu::GpuPreferences gpu_preferences;

  video_encode_accelerator_ =
      media::GpuVideoEncodeAcceleratorFactory::CreateVEA(
          input_format, input_visible_size, output_profile, initial_bitrate,
          this, gpu_preferences);

  if (!video_encode_accelerator_)
    return false;
  return true;
}

}  // namespace remoting
