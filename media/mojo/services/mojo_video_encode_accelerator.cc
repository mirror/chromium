// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_video_encode_accelerator.h"

#include <memory>

#include "base/logging.h"
#include "media/base/bind_to_current_loop.h"
#include "media/gpu/gpu_video_encode_accelerator_factory.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace media {

// static
void MojoVideoEncodeAccelerator::Create(
    media::mojom::VideoEncodeAcceleratorRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<MojoVideoEncodeAccelerator>(),
                          std::move(request));
}

MojoVideoEncodeAccelerator::MojoVideoEncodeAccelerator()
    : output_buffer_size_(0), weak_factory_(this) {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

MojoVideoEncodeAccelerator::~MojoVideoEncodeAccelerator() {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void MojoVideoEncodeAccelerator::Initialize(
    media::VideoPixelFormat input_format,
    const gfx::Size& input_visible_size,
    media::VideoCodecProfile output_profile,
    uint32_t initial_bitrate,
    mojom::VideoEncodeAcceleratorClientPtr client) {
  DVLOG(1) << __func__
           << " input_format=" << VideoPixelFormatToString(input_format)
           << ", input_visible_size=" << input_visible_size.ToString()
           << ", output_profile=" << GetProfileName(output_profile)
           << ", initial_bitrate=" << initial_bitrate;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(!encoder_);
  DCHECK_EQ(PIXEL_FORMAT_I420, input_format) << "Only I420 format supported";

  // TODO(mcasas): get it from someplace.
  gpu::GpuPreferences gpu_preferences;

  encoder_ = GpuVideoEncodeAcceleratorFactory::CreateVEA(
      input_format, input_visible_size, output_profile, initial_bitrate, this,
      gpu_preferences);
  if (!encoder_) {
    DLOG(ERROR) << __func__ << " Error creating VEA";
    return;
  }

  if (encoder_->TryToSetupEncodeOnSeparateThread(
          weak_factory_.GetWeakPtr(), base::ThreadTaskRunnerHandle::Get())) {
    DLOG(ERROR) << __func__ << " Error setting up VEA";
    return;
  }
  DCHECK(client);
  vea_client_ = std::move(client);
  return;
}

void MojoVideoEncodeAccelerator::Encode(const scoped_refptr<VideoFrame>& frame,
                                        bool force_keyframe,
                                        const EncodeCallback& callback) {
  DVLOG(1) << __func__ << " tstamp=" << frame->timestamp();
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!encoder_)
    return;

  frame->AddDestructionObserver(BindToCurrentLoop(std::move(callback)));
  encoder_->Encode(frame, force_keyframe);
}

void MojoVideoEncodeAccelerator::UseOutputBitstreamBuffer(
    int32_t bitstream_buffer_id,
    mojo::ScopedSharedBufferHandle buffer) {
  DVLOG(1) << __func__ << " bitstream_buffer_id= " << bitstream_buffer_id;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(buffer.is_valid());

  if (!encoder_)
    return;
  if (bitstream_buffer_id < 0) {
    DLOG(ERROR) << __func__ << " bitstream_buffer_id=" << bitstream_buffer_id
                << " must be >= 0";
    NotifyError(::media::VideoEncodeAccelerator::kPlatformFailureError);
    return;
  }

  base::SharedMemoryHandle handle;
  size_t memory_size = 0;
  bool read_only = false;
  auto result = mojo::UnwrapSharedMemoryHandle(std::move(buffer), &handle,
                                               &memory_size, &read_only);
  DCHECK_EQ(MOJO_RESULT_OK, result);
  DCHECK_GT(memory_size, 0u);
  if (memory_size > output_buffer_size_) {
    DLOG(ERROR) << __func__ << " bitstream_buffer_id=" << bitstream_buffer_id
                << " has a size of " << memory_size
                << "B, larger than expected " << output_buffer_size_ << "B";
    NotifyError(::media::VideoEncodeAccelerator::kPlatformFailureError);
    return;
  }

  encoder_->UseOutputBitstreamBuffer(
      BitstreamBuffer(bitstream_buffer_id, handle, memory_size));
}

void MojoVideoEncodeAccelerator::RequestEncodingParametersChange(
    uint32_t bitrate,
    uint32_t framerate) {
  DVLOG(1) << __func__ << " bitrate= " << bitrate << " framerate " << framerate;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!encoder_)
    return;
  encoder_->RequestEncodingParametersChange(bitrate, framerate);
}

void MojoVideoEncodeAccelerator::RequireBitstreamBuffers(
    unsigned int input_count,
    const gfx::Size& input_coded_size,
    size_t output_buffer_size) {
  DVLOG(1) << __func__ << " input_count= " << input_count
           << " input_coded_size= " << input_coded_size.ToString()
           << " output_buffer_size" << output_buffer_size;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(vea_client_);

  output_buffer_size_ = output_buffer_size;

  vea_client_->RequireBitstreamBuffers(input_count, input_coded_size,
                                       output_buffer_size);
}

void MojoVideoEncodeAccelerator::BitstreamBufferReady(
    int32_t bitstream_buffer_id,
    size_t payload_size,
    bool key_frame,
    base::TimeDelta timestamp) {
  DVLOG(1) << __func__ << " bitstream_buffer_id=" << bitstream_buffer_id
           << ", payload_size=" << payload_size
           << "B,  key_frame=" << key_frame;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(vea_client_);

  vea_client_->BitstreamBufferReady(bitstream_buffer_id, payload_size,
                                    key_frame, timestamp);
}

void MojoVideoEncodeAccelerator::NotifyError(
    ::media::VideoEncodeAccelerator::Error error) {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(vea_client_);

  // TODO(mcasas): Use EnumTraits, https://crbug.com/736517
  mojom::VideoEncodeAcceleratorError mojo_error =
      mojom::VideoEncodeAcceleratorError::PLATFORM_FAILURE_ERROR;
  switch (error) {
    case ::media::VideoEncodeAccelerator::kIllegalStateError:
      mojo_error = mojom::VideoEncodeAcceleratorError::ILLEGAL_STATE_ERROR;
      break;
    case ::media::VideoEncodeAccelerator::kInvalidArgumentError:
      mojo_error = mojom::VideoEncodeAcceleratorError::INVALID_ARGUMENT_ERROR;
      break;
    case ::media::VideoEncodeAccelerator::kPlatformFailureError:
      mojo_error = mojom::VideoEncodeAcceleratorError::PLATFORM_FAILURE_ERROR;
      break;
  }
  vea_client_->NotifyError(mojo_error);
}

}  // namespace media
