// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/video_capture/example_plugin/video_frame_forwarder.h"

#include "services/video_capture/public/interfaces/constants.mojom.h"

namespace video_capture {
namespace example_plugin {

VideoFrameForwarder::ProducerDelegator::ProducerDelegator(
    VideoFrameForwarder* forwarder)
    : forwarder_(forwarder) {}

void VideoFrameForwarder::ProducerDelegator::OnNewBufferHandle(
    int32_t buffer_id,
    mojo::ScopedSharedBufferHandle buffer_handle,
    OnNewBufferHandleCallback callback) {
  forwarder_->OnProducerNewBufferHandle(buffer_id, std::move(buffer_handle),
                                        std::move(callback));
}

void VideoFrameForwarder::ProducerDelegator::OnBufferRetired(
    int32_t buffer_id) {
  forwarder_->OnBufferRetired(buffer_id);
}

VideoFrameForwarder::VideoFrameForwarder() = default;

VideoFrameForwarder::~VideoFrameForwarder() = default;

void VideoFrameForwarder::SetTarget(mojom::VirtualDevicePtr target) {
  target_ = std::move(target);
}

mojom::Producer* VideoFrameForwarder::GetAsProducer() {
  if (!producer_delegator_)
    producer_delegator_ = std::make_unique<ProducerDelegator>(this);
  return producer_delegator_.get();
}

void VideoFrameForwarder::OnProducerNewBufferHandle(
    int32_t buffer_id,
    mojo::ScopedSharedBufferHandle buffer_handle,
    mojom::Producer::OnNewBufferHandleCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto handle_provider = std::make_unique<media::SharedMemoryHandleProvider>();
  handle_provider->InitFromMojoHandle(std::move(buffer_handle));
  outgoing_buffer_id_to_buffer_map_.insert(
      std::make_pair(buffer_id, std::move(handle_provider)));
  std::move(callback).Run();
}

void VideoFrameForwarder::OnProducerBufferRetired(int32_t buffer_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  outgoing_buffer_id_to_buffer_map_.erase(buffer_id);
}

void VideoFrameForwarder::OnNewBufferHandle(
    int32_t buffer_id,
    mojo::ScopedSharedBufferHandle buffer_handle) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto handle_provider = std::make_unique<media::SharedMemoryHandleProvider>();
  handle_provider->InitFromMojoHandle(std::move(buffer_handle));
  incoming_buffer_id_to_buffer_map_.insert(
      std::make_pair(buffer_id, std::move(handle_provider)));
}

void VideoFrameForwarder::OnFrameReadyInBuffer(
    int32_t buffer_id,
    int32_t frame_feedback_id,
    mojom::ScopedAccessPermissionPtr access_permission,
    media::mojom::VideoFrameInfoPtr frame_info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  target_->RequestFrameBuffer(
      frame_info->coded_size, frame_info->pixel_format,
      frame_info->storage_type,
      base::BindOnce(&VideoFrameForwarder::CopyFrameToVirtualDevice,
                     base::Unretained(this), buffer_id,
                     base::Passed(&access_permission),
                     base::Passed(&frame_info)));
}

void VideoFrameForwarder::OnBufferRetired(int32_t buffer_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  incoming_buffer_id_to_buffer_map_.erase(buffer_id);
}

void VideoFrameForwarder::OnError() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  LOG(ERROR) << "Capture from device has failed.";
}

void VideoFrameForwarder::OnLog(const std::string& message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(3) << "Log message received as video_capture::mojom::Receiver: "
          << message;
}

void VideoFrameForwarder::OnStarted() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  VLOG(3) << "Capture from device has started.";
}

void VideoFrameForwarder::OnStartedUsingGpuDecode() {}

void VideoFrameForwarder::CopyFrameToVirtualDevice(
    int32_t incoming_buffer_id,
    mojom::ScopedAccessPermissionPtr incoming_buffer_access_permission,
    media::mojom::VideoFrameInfoPtr frame_info,
    int32_t outgoing_buffer_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (outgoing_buffer_id == mojom::kInvalidBufferId) {
    // |target_| was unable to provide a buffer for us to write to. Probably
    // all buffers are already in use.
    LOG(WARNING) << "Dropping frame";
    return;
  }
  auto incoming_buffer =
      incoming_buffer_id_to_buffer_map_.at(incoming_buffer_id)
          ->GetHandleForInProcessAccess();
  auto outgoing_buffer =
      outgoing_buffer_id_to_buffer_map_.at(outgoing_buffer_id)
          ->GetHandleForInProcessAccess();
  CHECK(outgoing_buffer->mapped_size() >= incoming_buffer->mapped_size());
  memcpy(outgoing_buffer->data(), incoming_buffer->const_data(),
         incoming_buffer->mapped_size());

  // Example modification of frame data in I420 format. Set every 23th byte to
  // zero.
  for (uint8_t* data_pointer = outgoing_buffer->data();
       data_pointer < outgoing_buffer->data() + outgoing_buffer->mapped_size();
       data_pointer += 23) {
    *data_pointer = 0;
  }

  target_->OnFrameReadyInBuffer(outgoing_buffer_id, std::move(frame_info));
}

}  // namespace example_plugin
}  // namespace video_capture
