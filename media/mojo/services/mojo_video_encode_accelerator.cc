// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_video_encode_accelerator.h"

#include <memory>

#include "base/logging.h"
#include "media/gpu/gpu_video_encode_accelerator_factory.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace media {

// static
void MojoVideoEncodeAccelerator::Create(
    media::mojom::VideoEncodeAcceleratorRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<MojoVideoEncodeAccelerator>(),
                          std::move(request));
}

MojoVideoEncodeAccelerator::MojoVideoEncodeAccelerator() : weak_factory_(this) {
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

void MojoVideoEncodeAccelerator::Encode(int32_t buffer_id) {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void MojoVideoEncodeAccelerator::UseOutputBitstreamBuffer(
    int32_t buffer_id, mojo::ScopedSharedBufferHandle buffer) {
  DVLOG(1) << __func__ << " buffer.id()= " << buffer.id()
           << " buffer.size()= " << buffer.size() << "B";
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
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

  vea_client_->RequireBitstreamBuffers(input_count, input_coded_size,
                                       output_buffer_size);
}

void MojoVideoEncodeAccelerator::BitstreamBufferReady(
    int32_t bitstream_buffer_id,
    size_t payload_size,
    bool key_frame,
    base::TimeDelta timestamp) {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void MojoVideoEncodeAccelerator::NotifyError(
    ::media::VideoEncodeAccelerator::Error error) {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

}  // namespace media
