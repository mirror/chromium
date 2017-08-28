// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/shared_memory_buffer_tracker.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "ui/gfx/geometry/size.h"

namespace media {

SharedMemoryBufferTracker::SharedMemoryBufferTracker()
    : VideoCaptureBufferTracker(), map_ref_count_(0) {}

SharedMemoryBufferTracker::~SharedMemoryBufferTracker() {
  // If the tracker is being destroyed, there must be no outstanding
  // Handles. If this DCHECK() triggers, it means that either there is a logic
  // flaw in VideoCaptureBufferPoolImpl, or a client did not delete all of its
  // owned VideoCaptureBufferHandles before calling Pool::ReliquishXYZ().
  DCHECK_EQ(map_ref_count_, 0);
}

bool SharedMemoryBufferTracker::Init(const gfx::Size& dimensions,
                                     VideoPixelFormat format,
                                     VideoPixelStorage storage_type) {
  DVLOG(2) << __func__ << "allocating ShMem of " << dimensions.ToString();
  set_dimensions(dimensions);
  // |dimensions| can be 0x0 for trackers that do not require memory backing.
  set_max_pixel_count(dimensions.GetArea());
  set_pixel_format(format);
  set_storage_type(storage_type);
  mapped_size_ = VideoCaptureFormat(dimensions, 0.0f, format, storage_type)
                     .ImageAllocationSize();
  if (!mapped_size_)
    return true;
  DCHECK_EQ(map_ref_count_, 0);
  return shared_memory_.CreateAnonymous(mapped_size_);
}

std::unique_ptr<VideoCaptureBufferHandle>
SharedMemoryBufferTracker::GetMemoryMappedAccess() {
  bool need_to_map_memory;
  {
    base::AutoLock lock(map_ref_count_lock_);
    DCHECK_GE(map_ref_count_, 0);
    ++map_ref_count_;
    need_to_map_memory = (map_ref_count_ == 1);
  }
  if (need_to_map_memory)
    CHECK(shared_memory_.Map(mapped_size_));

  return std::make_unique<Handle>(this);
}

mojo::ScopedSharedBufferHandle SharedMemoryBufferTracker::GetHandleForTransit(
    bool read_only) {
  return mojo::WrapSharedMemoryHandle(
      base::SharedMemory::DuplicateHandle(shared_memory_.handle()),
      mapped_size_, read_only);
}

base::SharedMemoryHandle
SharedMemoryBufferTracker::GetNonOwnedSharedMemoryHandleForLegacyIPC() {
  return shared_memory_.handle();
}

void SharedMemoryBufferTracker::OnHandleDestroyed() {
  bool need_to_unmap_memory;
  {
    base::AutoLock lock(map_ref_count_lock_);
    DCHECK_GT(map_ref_count_, 0);
    --map_ref_count_;
    need_to_unmap_memory = (map_ref_count_ == 0);
  }
  if (need_to_unmap_memory)
    CHECK(shared_memory_.Unmap());
}

SharedMemoryBufferTracker::Handle::Handle(SharedMemoryBufferTracker* owner)
    : owner_(owner) {}

SharedMemoryBufferTracker::Handle::~Handle() {
  owner_->OnHandleDestroyed();
}

size_t SharedMemoryBufferTracker::Handle::mapped_size() const {
  return owner_->mapped_size_;
}

uint8_t* SharedMemoryBufferTracker::Handle::data() const {
  return static_cast<uint8_t*>(owner_->shared_memory_.memory());
}

const uint8_t* SharedMemoryBufferTracker::Handle::const_data() const {
  return static_cast<const uint8_t*>(owner_->shared_memory_.memory());
}

}  // namespace media
