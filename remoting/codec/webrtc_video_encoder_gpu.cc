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
const int kWebrtcVideoEncoderGpuOutputBufferCount = 4;
}

namespace remoting {

WebrtcVideoEncoderGpu::~WebrtcVideoEncoderGpu() {}

int WebrtcVideoEncoderGpu::GetFreeVideoFrameIndex() {
  if (free_frame_indices_.empty()) {
    int new_index = frames_.size();
    frames_.push_back(media::VideoFrame::CreateFrame(
        /*video_pixel_format_*/ media::VideoPixelFormat::PIXEL_FORMAT_I420,
        input_coded_size_, gfx::Rect(input_coded_size_), input_coded_size_,
        base::TimeDelta()));
    return new_index;
  } else {
    int index = free_frame_indices_.back();
    free_frame_indices_.pop_back();
    return index;
  }
}

void WebrtcVideoEncoderGpu::Encode(std::unique_ptr<webrtc::DesktopFrame> frame,
                                   const FrameParams& params,
                                   WebrtcVideoEncoder::EncodeCallback done) {
  DVLOG(3) << __func__ << " bitrate = " << params.bitrate_kbps << ", "
           << "duration = " << params.duration << ", "
           << "key_frame = " << params.key_frame;

  if (state_ == INITIALIZING) {
    DVLOG(3) << "Encoder initializing, dropping Encode() request.";
    // TODO(gusss) thunk or drop?
    return;
  }

  gfx::Size new_input_visible_size =
      gfx::Size(frame->size().width(), frame->size().height());
  // First check that our encoder is initialized to handle this size frame.
  if (!video_encode_accelerator_ ||
      new_input_visible_size != input_visible_size_) {
    DVLOG(3) << "Encoder not initialized for size "
             << new_input_visible_size.width() << "x"
             << new_input_visible_size.height() << ", initializing now.";

    input_visible_size_ = new_input_visible_size;

    // Drop all previous VideoFrames, as they are useless if the resolution
    // changed.
    frames_.clear();
    free_frame_indices_.clear();

    // TODO(gusss) can we somehow use:
    // media::GpuVideoEncodeAcceleratorFactory::GetSupportedProfiles()
    // use the above call to get the available input formats, pick a
    // TODO(gusss) how to calculate an estimate?
    // encoder won't initialize if this number is too high!
    uint32_t initial_bitrate =
        10000;  // input_visible_size_.GetArea() * 8 * 30;
    gpu::GpuPreferences gpu_preferences;
    video_encode_accelerator_ =
        media::GpuVideoEncodeAcceleratorFactory::CreateVEA(
            pixel_format_, input_visible_size_, codec_profile_, initial_bitrate,
            this, gpu_preferences);

    state_ = INITIALIZING;

    // thunk encode --> TODO(gusss) confirm that this is actually necessary! i
    // think it is because coded size may be different from the desktop size.

    return;
  }

  int free_frame_index = GetFreeVideoFrameIndex();
  scoped_refptr<media::VideoFrame> video_frame = frames_[free_frame_index];

  last_timestamp_ += params.duration;
  video_frame->set_timestamp(last_timestamp_);

  // Convert from ARGB to YUV.
  const uint8_t* rgb_data = frame->data();
  const int rgb_stride = frame->stride();
  const int y_stride = video_frame->stride(0);
  DCHECK_EQ(video_frame->stride(1), video_frame->stride(2));
  const int uv_stride = video_frame->stride(1);
  uint8_t* y_data = video_frame->data(0);
  uint8_t* u_data = video_frame->data(1);
  uint8_t* v_data = video_frame->data(2);
  libyuv::ARGBToI420(rgb_data, rgb_stride, y_data, y_stride, u_data, uv_stride,
                     v_data, uv_stride, video_frame->coded_size().width(),
                     video_frame->coded_size().height());

  // TODO(gusss) pad the frame to |input_coded_size_|

  // TODO(gusss) make sure this timestamp actually gets set.
  callbacks_[video_frame->timestamp()] = std::move(done);
  newly_freed_frames_[video_frame->timestamp()] = free_frame_index;

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

  // TODO(gusss) needed?
  required_input_frame_count_ = input_count;
  // TODO(gusss) input to Encode() must be padded to be of size
  // |input_coded_size|.
  input_coded_size_ = input_coded_size;
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

  // TODO(gusss) crop image down to visible size.

  LOG_ASSERT(newly_freed_frames_.find(timestamp) != newly_freed_frames_.end())
      << "Frame index to free not found for timestamp " << timestamp;
  int index_to_free = newly_freed_frames_[timestamp];
  free_frame_indices_.push_back(index_to_free);
  newly_freed_frames_.erase(timestamp);
  UseOutputBitstreamBufferId(index_to_free);

  LOG_ASSERT(callbacks_.find(timestamp) != callbacks_.end())
      << "Callback not found for timestamp " << timestamp;
  WebrtcVideoEncoder::EncodeCallback callback =
      std::move(callbacks_[timestamp]);
  callbacks_.erase(timestamp);
  std::move(callback).Run(std::move(encoded_frame));
}

void WebrtcVideoEncoderGpu::NotifyError(
    media::VideoEncodeAccelerator::Error error) {
  NOTREACHED();
}

WebrtcVideoEncoderGpu::WebrtcVideoEncoderGpu(
    media::VideoPixelFormat pixel_format,
    media::VideoCodecProfile codec_profile)
    : state_(UNINITIALIZED),
      pixel_format_(pixel_format),
      codec_profile_(codec_profile) {}

void WebrtcVideoEncoderGpu::UseOutputBitstreamBufferId(
    int bitstream_buffer_id) {
  DVLOG(3) << __func__ << " id = " << bitstream_buffer_id;

  video_encode_accelerator_->UseOutputBitstreamBuffer(media::BitstreamBuffer(
      bitstream_buffer_id, output_buffers_[bitstream_buffer_id]->handle(),
      output_buffers_[bitstream_buffer_id]->mapped_size()));
}

// static
std::unique_ptr<WebrtcVideoEncoderGpu> WebrtcVideoEncoderGpu::CreateForH264() {
  DVLOG(3) << __func__;

  // TODO(gusss) picked an arbitrary h264 profile. should maybe use
  // media::GpuVideoEncodeAcceleratorFactories::SupportedProfiles() or whatever
  // to see what h264 profiles are available.
  // TODO(gusss) picking the format needed for HW encoding on windows. not
  // really sure how to do this otherwise - i.e. how to find out what input
  // format is required
  std::unique_ptr<WebrtcVideoEncoderGpu> encoder =
      base::WrapUnique(new WebrtcVideoEncoderGpu(
          media::VideoPixelFormat::PIXEL_FORMAT_I420,
          media::VideoCodecProfile::H264PROFILE_BASELINE));
  // TODO(gusss) initialization is delayed until we know more about the input
  // frames i.e. until after encode is called for the first time
  /*
  if (encoder->Initialize())
    return encoder;
  return nullptr;*/
  return encoder;
}

bool WebrtcVideoEncoderGpu::Initialize() {
  DVLOG(3) << __func__;

  // TODO(gusss) picking arbitrary values for now. Where do these come from in
  // the future?
  // From Sergey: to get input format, we might just delay encoder
  // initialization until we have a number of frames.
  media::VideoPixelFormat input_format =
      media::VideoPixelFormat::PIXEL_FORMAT_I420;
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
