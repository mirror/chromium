// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/clients/mojo_video_encode_accelerator_host.h"

#include "base/logging.h"
#include "gpu/config/gpu_info.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "media/base/video_frame.h"
#include "media/gpu/gpu_video_accelerator_util.h"
#include "media/mojo/interfaces/video_encode_accelerator.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace media {

namespace {

// File-static mojom::VideoEncodeAcceleratorClient implementation to trampoline
// method calls to its |client_|. Note that this class is thread hostile when
// bound.
class VideoEncodeAcceleratorClient
    : public mojom::VideoEncodeAcceleratorClient {
 public:
  explicit VideoEncodeAcceleratorClient(VideoEncodeAccelerator::Client* client);
  ~VideoEncodeAcceleratorClient() override = default;

  // mojom::VideoEncodeAcceleratorClient impl.
  void RequireBitstreamBuffers(uint32_t input_count,
                               const gfx::Size& input_coded_size,
                               uint32_t output_buffer_size) override;
  void NotifyInputDone(int32_t buffer_id) override;
  void BitstreamBufferReady(int32_t buffer_id,
                            uint32_t payload_size,
                            bool key_frame,
                            base::TimeDelta timestamp) override;
  void NotifyError(mojom::VideoEncodeAcceleratorError error) override;

 private:
  VideoEncodeAccelerator::Client* client_;

  DISALLOW_COPY_AND_ASSIGN(VideoEncodeAcceleratorClient);
};

VideoEncodeAcceleratorClient::VideoEncodeAcceleratorClient(
    VideoEncodeAccelerator::Client* client)
    : client_(client) {
  DCHECK(client_);
}

void VideoEncodeAcceleratorClient::RequireBitstreamBuffers(
    uint32_t input_count,
    const gfx::Size& input_coded_size,
    uint32_t output_buffer_size) {
  DVLOG(1) << __func__ << " input_count= " << input_count
           << " input_coded_size= " << input_coded_size.ToString()
           << " output_buffer_size" << output_buffer_size;
  if (!client_)
    return;
  client_->RequireBitstreamBuffers(input_count, input_coded_size,
                                   output_buffer_size);
}

void VideoEncodeAcceleratorClient::NotifyInputDone(int32_t buffer_id) {
  DVLOG(1) << __func__;

  // TODO(mcasas): parse and decide if this is relevant in a post-IPC world.
}

void VideoEncodeAcceleratorClient::BitstreamBufferReady(
    int32_t buffer_id,
    uint32_t payload_size,
    bool key_frame,
    base::TimeDelta timestamp) {
  DVLOG(1) << __func__ << " buffer_id=" << buffer_id
           << ", payload_size=" << payload_size
           << "B,  key_frame=" << key_frame;
  if (!client_)
    return;
  client_->BitstreamBufferReady(buffer_id, payload_size, key_frame, timestamp);
}

void VideoEncodeAcceleratorClient::NotifyError(
    mojom::VideoEncodeAcceleratorError mojo_error) {
  DVLOG(1) << __func__;
  if (!client_)
    return;

  // TODO(mcasas): Use EnumTraits, https://crbug.com/736517
  ::media::VideoEncodeAccelerator::Error error;
  switch (mojo_error) {
    case mojom::VideoEncodeAcceleratorError::ILLEGAL_STATE_ERROR:
      error = ::media::VideoEncodeAccelerator::kIllegalStateError;
      break;
    case mojom::VideoEncodeAcceleratorError::INVALID_ARGUMENT_ERROR:
      error = ::media::VideoEncodeAccelerator::kInvalidArgumentError;
      break;
    case mojom::VideoEncodeAcceleratorError::PLATFORM_FAILURE_ERROR:
      error = ::media::VideoEncodeAccelerator::kPlatformFailureError;
      break;
  }
  client_->NotifyError(error);
}

}  // anonymous namespace

MojoVideoEncodeAcceleratorHost::MojoVideoEncodeAcceleratorHost(
    mojom::VideoEncodeAcceleratorPtr vea,
    gpu::CommandBufferProxyImpl* impl)
    : vea_(std::move(vea)), channel_(impl->channel()) {
  DVLOG(1) << __func__;
  DCHECK(vea_);
  DCHECK(channel_);
}

VideoEncodeAccelerator::SupportedProfiles
MojoVideoEncodeAcceleratorHost::GetSupportedProfiles() {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  DCHECK(channel_);
  return GpuVideoAcceleratorUtil::ConvertGpuToMediaEncodeProfiles(
      channel_->gpu_info().video_encode_accelerator_supported_profiles);
}

bool MojoVideoEncodeAcceleratorHost::Initialize(
    VideoPixelFormat input_format,
    const gfx::Size& input_visible_size,
    VideoCodecProfile output_profile,
    uint32_t initial_bitrate,
    Client* client) {
  DVLOG(1) << __func__
           << " input_format=" << VideoPixelFormatToString(input_format)
           << ", input_visible_size=" << input_visible_size.ToString()
           << ", output_profile=" << GetProfileName(output_profile)
           << ", initial_bitrate=" << initial_bitrate;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Get a mojom::VideoEncodeAcceleratorClient bound to a local implementation
  // (VideoEncodeAcceleratorClient) and send the pointer remotely.
  mojo::MakeStrongBinding(
      base::MakeUnique<VideoEncodeAcceleratorClient>(client),
      mojo::MakeRequest(&vea_client_));

  vea_->Initialize(input_format, input_visible_size, output_profile,
                   initial_bitrate, std::move(vea_client_));

  return true;
}

void MojoVideoEncodeAcceleratorHost::Encode(
    const scoped_refptr<VideoFrame>& frame,
    bool force_keyframe) {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(PIXEL_FORMAT_I420, frame->format());
  DCHECK_EQ(VideoFrame::STORAGE_SHMEM, frame->storage_type());

  DCHECK(vea_.is_bound());
  vea_->Encode(0, frame, force_keyframe);
}

void MojoVideoEncodeAcceleratorHost::UseOutputBitstreamBuffer(
    const BitstreamBuffer& buffer) {
  DVLOG(1) << __func__ << " buffer.id()= " << buffer.id()
           << " buffer.size()= " << buffer.size() << "B";
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  mojo::ScopedSharedBufferHandle buffer_handle = mojo::WrapSharedMemoryHandle(
      buffer.handle(), buffer.size(), true /* read_only */);

  vea_->UseOutputBitstreamBuffer(buffer.id(), std::move(buffer_handle));
}

void MojoVideoEncodeAcceleratorHost::RequestEncodingParametersChange(
    uint32_t bitrate,
    uint32_t framerate) {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void MojoVideoEncodeAcceleratorHost::Destroy() {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  vea_.reset();
}

MojoVideoEncodeAcceleratorHost::~MojoVideoEncodeAcceleratorHost() {
  DVLOG(1) << __func__;
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

}  // namespace media
