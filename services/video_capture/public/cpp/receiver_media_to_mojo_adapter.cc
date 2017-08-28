// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/video_capture/public/cpp/receiver_media_to_mojo_adapter.h"

#include "base/logging.h"
#include "base/sequence_checker.h"
#include "media/capture/video/video_capture_buffer_handle.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/public/cpp/system/platform_handle.h"

namespace {

class MojoBufferHandleProvider
    : public media::VideoCaptureDevice::Client::Buffer::HandleProvider {
 public:
  MojoBufferHandleProvider(mojo::ScopedSharedBufferHandle buffer_handle)
      : buffer_handle_(std::move(buffer_handle)) {
    // It's okay to be constructed at any time, but all public methods must be
    // invoked in-sequence or else locking would be required throughout the
    // implementation.
    DETACH_FROM_SEQUENCE(sequence_checker_);
  }

  ~MojoBufferHandleProvider() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    DCHECK_EQ(map_ref_count_, 0);
  }

  // Implementation of Buffer::HandleProvider:
  mojo::ScopedSharedBufferHandle GetHandleForInterProcessTransit() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return buffer_handle_->Clone();
  }
  base::SharedMemoryHandle GetNonOwnedSharedMemoryHandleForLegacyIPC()
      override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    LazyUnwrapHandleAndMapMemory();
    return shared_memory_->handle();
  }
  std::unique_ptr<media::VideoCaptureBufferHandle> GetHandleForInProcessAccess()
      override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    LazyUnwrapHandleAndMapMemory();
    return std::make_unique<Handle>(this);
  }

 private:
  // Simple "accessor" to provide a view of the mapped memory.
  class Handle : public media::VideoCaptureBufferHandle {
   public:
    Handle(MojoBufferHandleProvider* owner) : owner_(owner) {
      DCHECK_CALLED_ON_VALID_SEQUENCE(owner_->sequence_checker_);
      DCHECK_GE(owner_->map_ref_count_++, 0);
    }

    ~Handle() final {
      DCHECK_CALLED_ON_VALID_SEQUENCE(owner_->sequence_checker_);
      DCHECK_GT(owner_->map_ref_count_--, 0);
    }

    size_t mapped_size() const final { return owner_->memory_size_; }
    uint8_t* data() const final {
      return static_cast<uint8_t*>(owner_->shared_memory_->memory());
    }
    const uint8_t* const_data() const final {
      return static_cast<const uint8_t*>(owner_->shared_memory_->memory());
    }

   private:
    MojoBufferHandleProvider* const owner_;
  };

  void LazyUnwrapHandleAndMapMemory() {
    if (shared_memory_.has_value())
      return;  // already done before

    base::SharedMemoryHandle memory_handle;
    const MojoResult result =
        mojo::UnwrapSharedMemoryHandle(buffer_handle_->Clone(), &memory_handle,
                                       &memory_size_, &read_only_flag_);
    DCHECK_EQ(MOJO_RESULT_OK, result);
    DCHECK_GT(memory_size_, 0u);

    shared_memory_.emplace(memory_handle, read_only_flag_);
    if (!shared_memory_->Map(memory_size_)) {
      DLOG(ERROR) << "LazyUnwrapHandleAndMapMemory: Map failed.";
    }
  }

  mojo::ScopedSharedBufferHandle buffer_handle_;
  base::Optional<base::SharedMemory> shared_memory_;
  size_t memory_size_ = 0;
  bool read_only_flag_ = false;

#if DCHECK_IS_ON()
  int map_ref_count_ = 0;
#endif

  SEQUENCE_CHECKER(sequence_checker_);
};

class ScopedAccessPermissionMojoToMediaAdapter
    : public media::VideoCaptureDevice::Client::Buffer::ScopedAccessPermission {
 public:
  ScopedAccessPermissionMojoToMediaAdapter(
      video_capture::mojom::ScopedAccessPermissionPtr access_permission)
      : access_permission_(std::move(access_permission)) {}

 private:
  video_capture::mojom::ScopedAccessPermissionPtr access_permission_;
};

}  // anonymous namespace

namespace video_capture {

ReceiverMediaToMojoAdapter::ReceiverMediaToMojoAdapter(
    std::unique_ptr<media::VideoFrameReceiver> receiver)
    : receiver_(std::move(receiver)) {}

ReceiverMediaToMojoAdapter::~ReceiverMediaToMojoAdapter() = default;

void ReceiverMediaToMojoAdapter::OnNewBufferHandle(
    int32_t buffer_id,
    mojo::ScopedSharedBufferHandle buffer_handle) {
  receiver_->OnNewBufferHandle(
      buffer_id,
      base::MakeUnique<MojoBufferHandleProvider>(std::move(buffer_handle)));
}

void ReceiverMediaToMojoAdapter::OnFrameReadyInBuffer(
    int32_t buffer_id,
    int32_t frame_feedback_id,
    mojom::ScopedAccessPermissionPtr access_permission,
    media::mojom::VideoFrameInfoPtr frame_info) {
  receiver_->OnFrameReadyInBuffer(
      buffer_id, frame_feedback_id,
      base::MakeUnique<ScopedAccessPermissionMojoToMediaAdapter>(
          std::move(access_permission)),
      std::move(frame_info));
}

void ReceiverMediaToMojoAdapter::OnBufferRetired(int32_t buffer_id) {
  receiver_->OnBufferRetired(buffer_id);
}

void ReceiverMediaToMojoAdapter::OnError() {
  receiver_->OnError();
}

void ReceiverMediaToMojoAdapter::OnLog(const std::string& message) {
  receiver_->OnLog(message);
}

void ReceiverMediaToMojoAdapter::OnStarted() {
  receiver_->OnStarted();
}

void ReceiverMediaToMojoAdapter::OnStartedUsingGpuDecode() {
  receiver_->OnStartedUsingGpuDecode();
}

}  // namespace video_capture
