// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/video_capture/virtual_device_mojo_adapter.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/scoped_callback_runner.h"
#include "media/capture/video/video_capture_buffer_pool_impl.h"
#include "media/capture/video/video_capture_buffer_tracker_factory_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace {

// The maximum number of video frame buffers in-flight at any one time
// If all buffers are still in use by consumers when new frames are produced
// those frames get dropped.
static const int kMaxBufferCount = 3;

}  // anonymous namespace

namespace video_capture {

namespace {

// Buffer access permission for the consumer.
class ScopedBufferConsumerAccessPermission {
 public:
  ScopedBufferConsumerAccessPermission(
      scoped_refptr<media::VideoCaptureBufferPool> buffer_pool,
      int buffer_id)
      : buffer_pool_(std::move(buffer_pool)), buffer_id_(buffer_id) {}

  ~ScopedBufferConsumerAccessPermission() {
    buffer_pool_->RelinquishConsumerHold(buffer_id_, 1 /* num_clients */);
  }

 private:
  const scoped_refptr<media::VideoCaptureBufferPool> buffer_pool_;
  const int buffer_id_;
};

class ScopedAccessPermissionMojoAdapter
    : public video_capture::mojom::ScopedAccessPermission {
 public:
  ScopedAccessPermissionMojoAdapter(
      std::unique_ptr<ScopedBufferConsumerAccessPermission> access_permission)
      : access_permission_(std::move(access_permission)) {}

 private:
  std::unique_ptr<ScopedBufferConsumerAccessPermission> access_permission_;
};

}  // anonymous namespace

VirtualDeviceMojoAdapter::VirtualDeviceMojoAdapter(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref,
    const media::VideoCaptureDeviceInfo& device_info)
    : service_ref_(std::move(service_ref)),
      device_info_(device_info),
      buffer_pool_(new media::VideoCaptureBufferPoolImpl(
          base::MakeUnique<media::VideoCaptureBufferTrackerFactoryImpl>(),
          max_buffer_pool_buffer_count())) {}

VirtualDeviceMojoAdapter::~VirtualDeviceMojoAdapter() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

int VirtualDeviceMojoAdapter::max_buffer_pool_buffer_count() {
  return kMaxBufferCount;
}

void VirtualDeviceMojoAdapter::GetFrameBuffer(
    const gfx::Size& dimension,
    media::VideoPixelFormat pixel_format,
    media::VideoPixelStorage pixel_storage,
    GetFrameBufferCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  int buffer_id_to_drop = media::VideoCaptureBufferPool::kInvalidId;
  const int buffer_id = buffer_pool_->ReserveForProducer(
      dimension, pixel_format, pixel_storage, 0 /* frame_feedback_id */,
      &buffer_id_to_drop);

  // Remove dropped buffer if there is one.
  if (buffer_id_to_drop != media::VideoCaptureBufferPool::kInvalidId) {
    auto entry_iter = std::find(known_buffer_ids_.begin(),
                                known_buffer_ids_.end(), buffer_id_to_drop);
    if (entry_iter != known_buffer_ids_.end()) {
      known_buffer_ids_.erase(entry_iter);
      if (receiver_.is_bound()) {
        receiver_->OnBufferRetired(buffer_id_to_drop);
      }
    }
  }

  // No buffer available.
  if (buffer_id == media::VideoCaptureBufferPool::kInvalidId) {
    std::move(callback).Run(-1, mojo::ScopedSharedBufferHandle());
    return;
  }

  if (!base::ContainsValue(known_buffer_ids_, buffer_id)) {
    if (receiver_.is_bound()) {
      receiver_->OnNewBufferHandle(
          buffer_id, buffer_pool_->GetHandleForInterProcessTransit(
                         buffer_id, true /* read_only */));
    }
    known_buffer_ids_.push_back(buffer_id);
  }
  std::move(callback).Run(buffer_id,
                          buffer_pool_->GetHandleForInterProcessTransit(
                              buffer_id, false /* read_only */));
}

void VirtualDeviceMojoAdapter::OnFrameReadyInBuffer(
    int32_t buffer_id,
    ::media::mojom::VideoFrameInfoPtr frame_info) {
  // Unknown buffer ID.
  if (!base::ContainsValue(known_buffer_ids_, buffer_id)) {
    return;
  }

  // Notify receiver if there is one.
  if (receiver_.is_bound()) {
    buffer_pool_->HoldForConsumers(buffer_id, 1 /* num_clients */);
    auto access_permission =
        std::make_unique<ScopedBufferConsumerAccessPermission>(buffer_pool_,
                                                               buffer_id);
    mojom::ScopedAccessPermissionPtr access_permission_proxy;
    mojo::MakeStrongBinding<mojom::ScopedAccessPermission>(
        std::make_unique<ScopedAccessPermissionMojoAdapter>(
            std::move(access_permission)),
        mojo::MakeRequest(&access_permission_proxy));
    receiver_->OnFrameReadyInBuffer(buffer_id, 0 /* frame_feedback_id */,
                                    std::move(access_permission_proxy),
                                    std::move(frame_info));
  }
  buffer_pool_->RelinquishProducerReservation(buffer_id);
}

void VirtualDeviceMojoAdapter::Start(
    const media::VideoCaptureParams& requested_settings,
    mojom::ReceiverPtr receiver) {
  DCHECK(thread_checker_.CalledOnValidThread());
  receiver.set_connection_error_handler(
      base::Bind(&VirtualDeviceMojoAdapter::OnReceiverConnectionErrorOrClose,
                 base::Unretained(this)));
  receiver_ = std::move(receiver);
  receiver_->OnStarted();

  // Notify receiver of known buffers */
  for (auto buffer_id : known_buffer_ids_) {
    receiver_->OnNewBufferHandle(buffer_id,
                                 buffer_pool_->GetHandleForInterProcessTransit(
                                     buffer_id, true /* read_only */));
  }
}

void VirtualDeviceMojoAdapter::OnReceiverReportingUtilization(
    int32_t frame_feedback_id,
    double utilization) {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void VirtualDeviceMojoAdapter::RequestRefreshFrame() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void VirtualDeviceMojoAdapter::MaybeSuspend() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void VirtualDeviceMojoAdapter::Resume() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void VirtualDeviceMojoAdapter::GetPhotoState(GetPhotoStateCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
  std::move(callback).Run(nullptr);
}

void VirtualDeviceMojoAdapter::SetPhotoOptions(
    media::mojom::PhotoSettingsPtr settings,
    SetPhotoOptionsCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void VirtualDeviceMojoAdapter::TakePhoto(TakePhotoCallback callback) {
  DCHECK(thread_checker_.CalledOnValidThread());
}

void VirtualDeviceMojoAdapter::Stop() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!receiver_.is_bound())
    return;
  // Unsubscribe from connection error callbacks.
  receiver_.set_connection_error_handler(base::Closure());
  receiver_.reset();
}

void VirtualDeviceMojoAdapter::OnReceiverConnectionErrorOrClose() {
  DCHECK(thread_checker_.CalledOnValidThread());
  Stop();
}

}  // namespace video_capture
