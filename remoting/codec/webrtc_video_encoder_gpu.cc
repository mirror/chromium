// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/codec/webrtc_video_encoder_gpu.h"

#include "base/memory/ptr_util.h"
#include "gpu/command_buffer/service/gpu_preferences.h"
#include "media/gpu/gpu_video_encode_accelerator_factory.h"
#include "third_party/libyuv/include/libyuv/convert_from_argb.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_frame.h"

namespace webrtc {
class DesktopFrame;
}

namespace {
// we're encoding one at a time - not much reason for this to be more than 1.
const int kWebrtcVideoEncoderGpuOutputBufferCount = 1;
}  // namespace

namespace remoting {

WebrtcVideoEncoderGpu::~WebrtcVideoEncoderGpu() {}

void WebrtcVideoEncoderGpu::UseOutputBitstreamBufferId(
    int32_t bitstream_buffer_id) {
  DVLOG(3) << __func__ << " id=" << bitstream_buffer_id;
  video_encode_accelerator_->UseOutputBitstreamBuffer(media::BitstreamBuffer(
      bitstream_buffer_id, output_buffers_[bitstream_buffer_id]->handle(),
      // TODO(gusss): they're not mapped beforehand. where should the size be
      // coming from?
      //output_buffers_[bitstream_buffer_id]->mapped_size()));
      output_buffer_size_));
}

void WebrtcVideoEncoderGpu::Encode(std::unique_ptr<webrtc::DesktopFrame> frame,
                                   const FrameParams& params,
                                   WebrtcVideoEncoder::EncodeCallback done) {
  DVLOG(3) << __func__ << " bitrate = " << params.bitrate_kbps << ", "
           << "duration = " << params.duration << ", "
           << "key_frame = " << params.key_frame;

  if (state_ == UNINITIALIZED) {
    DVLOG(3) << __func__ << " Currently uninitialized. Initializing.";
    input_visible_size_ =
        gfx::Size(frame->size().width(), frame->size().height());
    Initialize(input_visible_size_);
    pending_callback_ = std::move(done);
    state_ = INITIALIZING;
    return;
  }

  // TODO(gusss) is this right?
  scoped_refptr<media::VideoFrame> video_frame = media::VideoFrame::CreateFrame(
      media::VideoPixelFormat::PIXEL_FORMAT_I420, input_coded_size_,
      gfx::Rect(input_visible_size_), input_visible_size_, base::TimeDelta());

  base::TimeDelta new_timestamp = previous_timestamp_ + params.duration;
  video_frame->set_timestamp(new_timestamp);
  previous_timestamp_ = new_timestamp;

  // Convert from ARGB to YUV.
  const uint8_t* rgb_data = frame->data();
  const int rgb_stride = frame->stride();
  const int y_stride = video_frame->stride(0);
  DCHECK_EQ(video_frame->stride(1), video_frame->stride(2));
  const int uv_stride = video_frame->stride(1);
  uint8_t* y_data = video_frame->data(0);
  uint8_t* u_data = video_frame->data(1);
  uint8_t* v_data = video_frame->data(2);
  // TODO(gusss) changed from coded size to visible size - which is right?
  libyuv::ARGBToI420(rgb_data, rgb_stride, y_data, y_stride, u_data, uv_stride,
                     v_data, uv_stride, video_frame->visible_rect().width(),
                     video_frame->visible_rect().height());

  // TODO(gusss) make sure this timestamp actually gets set.
  callbacks_[video_frame->timestamp()] = std::move(done);

  video_encode_accelerator_->Encode(video_frame, false);
}

void WebrtcVideoEncoderGpu::RequireBitstreamBuffers(
    unsigned int input_count,
    const gfx::Size& input_coded_size,
    size_t output_buffer_size) {
  DVLOG(3) << __func__ << ", "
           << "input_count = " << input_count << ", "
           << "input_coded_size = " << input_coded_size.width() << "x"
           << input_coded_size.height() << ", "
           << "output_buffer_size = " << output_buffer_size;

  required_input_frame_count_ = input_count;
  input_coded_size_ = input_coded_size;
  // TODO(gusss) needed?
  output_buffer_size_ = output_buffer_size;

  output_buffers_.clear();

  for (unsigned int i = 0; i < kWebrtcVideoEncoderGpuOutputBufferCount; ++i) {
    auto shm = base::MakeUnique<base::SharedMemory>();
    LOG_ASSERT(shm->CreateAndMapAnonymous(output_buffer_size_));
    output_buffers_.push_back(std::move(shm));
  }

  for (size_t i = 0; i < output_buffers_.size(); ++i) {
    UseOutputBitstreamBufferId(i);
  }

  state_ = INITIALIZED;

  std::move(pending_callback_).Run(nullptr);
}

void WebrtcVideoEncoderGpu::BitstreamBufferReady(int32_t bitstream_buffer_id,
                                                 size_t payload_size,
                                                 bool key_frame,
                                                 base::TimeDelta timestamp) {
  DVLOG(3) << __func__ << " bitstream_buffer_id = " << bitstream_buffer_id
           << ", "
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

  UseOutputBitstreamBufferId(bitstream_buffer_id);

  LOG_ASSERT(callbacks_.find(timestamp) != callbacks_.end())
      << "Callback not found for timestamp " << timestamp;
  WebrtcVideoEncoder::EncodeCallback callback =
      std::move(callbacks_[timestamp]);
  callbacks_.erase(timestamp);
  std::move(callback).Run(std::move(encoded_frame));
}

void WebrtcVideoEncoderGpu::NotifyError(
    media::VideoEncodeAccelerator::Error error) {}

WebrtcVideoEncoderGpu::WebrtcVideoEncoderGpu(
    media::VideoCodecProfile codec_profile)
    : state_(UNINITIALIZED), codec_profile_(codec_profile) {}

// static
std::unique_ptr<WebrtcVideoEncoderGpu> WebrtcVideoEncoderGpu::CreateForH264() {
  DVLOG(3) << __func__;

  // TODO(gusss) picked an arbitrary h264 profile.
  return base::WrapUnique(new WebrtcVideoEncoderGpu(
      media::VideoCodecProfile::H264PROFILE_BASELINE));
}

bool WebrtcVideoEncoderGpu::Initialize(gfx::Size input_visible_size) {
  DVLOG(3) << __func__;

  // TODO(gusss) picking arbitrary values for now. Where do these come from in
  // the future?
  // From Sergey: to get input format, we might just delay encoder
  // initialization until we have a number of frames.
  media::VideoPixelFormat input_format =
      media::VideoPixelFormat::PIXEL_FORMAT_I420;
  uint32_t initial_bitrate = 8 * 1024 * 8;
  gpu::GpuPreferences gpu_preferences;

  gfx::Size input_visible_size_rounded(
      ((input_visible_size.width() >> 5) + 1) << 5,
      ((input_visible_size.height() >> 5) + 1) << 5);

  video_encode_accelerator_ =
      media::GpuVideoEncodeAcceleratorFactory::CreateVEA(
          input_format, input_visible_size_rounded, codec_profile_,
          initial_bitrate, this, gpu_preferences);

  if (!video_encode_accelerator_)
    return false;
  return true;
}

}  // namespace remoting
