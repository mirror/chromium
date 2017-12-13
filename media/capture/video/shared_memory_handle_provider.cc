// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/shared_memory_handle_provider.h"

#include "build/build_config.h"

namespace media {

SharedMemoryHandleProvider::SharedMemoryHandleProvider() {
#if DCHECK_IS_ON()
  map_ref_count_ = 0;
#endif
}

SharedMemoryHandleProvider::~SharedMemoryHandleProvider() {
  base::AutoLock lock(mapping_lock_);

  // If the tracker is being destroyed, there must be no outstanding
  // Handles. If this DCHECK() triggers, it means that either there is a logic
  // flaw in VideoCaptureBufferPoolImpl, or a client did not delete all of its
  // owned VideoCaptureBufferHandles before calling Pool::ReliquishXYZ().
#if DCHECK_IS_ON()
  DCHECK_EQ(map_ref_count_, 0);
#endif

  if (shared_memory_ && shared_memory_->memory()) {
    DVLOG(3) << __func__ << ": Unmapping memory for in-process access @"
             << shared_memory_->memory() << '.';
    CHECK(shared_memory_->Unmap());
  }
}

bool SharedMemoryHandleProvider::InitForSize(size_t size) {
#if DCHECK_IS_ON()
  DCHECK_EQ(map_ref_count_, 0);
#endif
  DCHECK(!shared_memory_);

  mojo::ScopedSharedBufferHandle buffer =
      mojo::SharedBufferHandle::Create(size);
  if (!buffer.is_valid())
    return false;
  return InitFromMojoHandle(std::move(buffer));
}

bool SharedMemoryHandleProvider::InitFromMojoHandle(
    mojo::ScopedSharedBufferHandle buffer_handle) {
#if DCHECK_IS_ON()
  DCHECK_EQ(map_ref_count_, 0);
#endif
  DCHECK(!shared_memory_);

#if defined(OS_POSIX) && !defined(OS_FUCHSIA) && !defined(OS_MACOSX)
  // NOTE: On Linux etc, base::SharedMemory maintains a separate internal FD for
  // read-only sharing. We attempt to extract a read-only clone of the input
  // handle before unwrapping it.
  mojo::ScopedSharedBufferHandle read_only_handle =
      buffer_handle->Clone(mojo::SharedBufferHandle::AccessMode::READ_ONLY);
#endif

  base::SharedMemoryHandle shm_handle;
  mojo::UnwrappedSharedMemoryHandleProtection protection;
  MojoResult result = mojo::UnwrapSharedMemoryHandle(
      std::move(buffer_handle), &shm_handle, nullptr, &protection);
  if (result != MOJO_RESULT_OK)
    return false;

  if (protection == mojo::UnwrappedSharedMemoryHandleProtection::kReadOnly) {
    // The input handle was already read-only, so we have everything we can use.
    shared_memory_.emplace(shm_handle, true /* read_only */);
    return true;
  }

#if defined(OS_POSIX) && !defined(OS_FUCHSIA) && !defined(OS_MACOSX)
  // Iff read-only handle extraction succeeded above, we also attempt to unwrap
  // that handle here. Upon success we'll initialize a base::SharedMemory
  // object which supports both writable and read-only mappings.
  base::SharedMemoryHandle read_only_shm_handle;
  if (read_only_handle.is_valid()) {
    result = mojo::UnwrapSharedMemoryHandle(
        std::move(read_only_handle), &read_only_shm_handle, nullptr, nullptr);
    if (result == MOJO_RESULT_OK) {
      shared_memory_.emplace(shm_handle, read_only_shm_handle);
      return true;
    }
  }
#endif

  // This base::SharedMemory object supports writable mapping, but does not
  // support vending read-only handles on some platforms.
  shared_memory_.emplace(shm_handle, false /* read_only */);
  return true;
}

mojo::ScopedSharedBufferHandle
SharedMemoryHandleProvider::GetHandleForInterProcessTransit(bool read_only) {
  if (read_only_flag_ && !read_only) {
    // Wanted read-write access, but read-only access is all that is available.
    NOTREACHED();
    return mojo::ScopedSharedBufferHandle();
  }
  if (read_only) {
    return mojo::WrapSharedMemoryHandle(
        shared_memory_->GetReadOnlyHandle(), mapped_size_,
        mojo::UnwrappedSharedMemoryHandleProtection::kReadOnly);
  } else {
    return mojo::WrapSharedMemoryHandle(
        base::SharedMemory::DuplicateHandle(shared_memory_->handle()),
        mapped_size_, mojo::UnwrappedSharedMemoryHandleProtection::kReadWrite);
  }
}

base::SharedMemoryHandle
SharedMemoryHandleProvider::GetNonOwnedSharedMemoryHandleForLegacyIPC() {
  return shared_memory_->handle();
}

std::unique_ptr<VideoCaptureBufferHandle>
SharedMemoryHandleProvider::GetHandleForInProcessAccess() {
  {
    base::AutoLock lock(mapping_lock_);
#if DCHECK_IS_ON()
    DCHECK_GE(map_ref_count_, 0);
    ++map_ref_count_;
#endif
    if (!shared_memory_->memory()) {
      CHECK(shared_memory_->Map(mapped_size_));
      DVLOG(3) << __func__ << ": Mapped memory for in-process access @"
               << shared_memory_->memory() << '.';
    }
  }

  return std::make_unique<Handle>(this);
}

#if DCHECK_IS_ON()
void SharedMemoryHandleProvider::OnHandleDestroyed() {
  base::AutoLock lock(mapping_lock_);
  DCHECK_GT(map_ref_count_, 0);
  --map_ref_count_;
}
#endif

SharedMemoryHandleProvider::Handle::Handle(SharedMemoryHandleProvider* owner)
    : owner_(owner) {}

SharedMemoryHandleProvider::Handle::~Handle() {
#if DCHECK_IS_ON()
  owner_->OnHandleDestroyed();
#endif
}

size_t SharedMemoryHandleProvider::Handle::mapped_size() const {
  return owner_->mapped_size_;
}

uint8_t* SharedMemoryHandleProvider::Handle::data() const {
  return static_cast<uint8_t*>(owner_->shared_memory_->memory());
}

const uint8_t* SharedMemoryHandleProvider::Handle::const_data() const {
  return static_cast<const uint8_t*>(owner_->shared_memory_->memory());
}

}  // namespace media
