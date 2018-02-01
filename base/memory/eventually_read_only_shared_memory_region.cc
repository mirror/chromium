// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/eventually_read_only_shared_memory_region.h"

#include "base/memory/shared_memory.h"
#include "build/build_config.h"

namespace base {

// static
EventuallyReadOnlySharedMemoryRegion
EventuallyReadOnlySharedMemoryRegion::Create(size_t size) {
  SharedMemoryCreateOptions options;
  options.size = size;
  options.share_read_only = true;
  SharedMemory shm;
  if (!shm.Create(options))
    return {};

  SharedMemoryHandle ro_handle = shm.GetReadOnlyHandle();
  return EventuallyReadOnlySharedMemoryRegion(shm.TakeHandle(), ro_handle);
}

// static
std::pair<SharedMemoryHandle, SharedMemoryHandle>
EventuallyReadOnlySharedMemoryRegion::TakeHandles(
    EventuallyReadOnlySharedMemoryRegion&& region) {
  SharedMemoryHandle rw_handle_copy = region.rw_handle_;
  region.rw_handle_ = SharedMemoryHandle();
  SharedMemoryHandle ro_handle_copy = region.ro_handle_;
  region.ro_handle_ = SharedMemoryHandle();
  return {rw_handle_copy, ro_handle_copy};
}

// static
ReadOnlySharedMemoryRegion
EventuallyReadOnlySharedMemoryRegion::ConvertToReadOnly(
    EventuallyReadOnlySharedMemoryRegion&& region) {
  SharedMemoryHandle rw_handle;
  SharedMemoryHandle ro_handle;
  std::tie(rw_handle, ro_handle) = TakeHandles(std::move(region));
  if (rw_handle.IsValid())
    rw_handle.Close();

  if (!ro_handle.IsValid())
    return {};

#if defined(OS_ANDROID)
  if (!ro_handle.SetRegionReadOnly())
    return {};
#endif  // defined(OS_ANDROID)

  return ReadOnlySharedMemoryRegion(ro_handle);
}

EventuallyReadOnlySharedMemoryRegion::EventuallyReadOnlySharedMemoryRegion() =
    default;

EventuallyReadOnlySharedMemoryRegion::EventuallyReadOnlySharedMemoryRegion(
    SharedMemoryHandle rw_handle,
    SharedMemoryHandle ro_handle)
    : rw_handle_(rw_handle), ro_handle_(ro_handle) {}

EventuallyReadOnlySharedMemoryRegion::EventuallyReadOnlySharedMemoryRegion(
    EventuallyReadOnlySharedMemoryRegion&& region) {
  std::tie(rw_handle_, ro_handle_) = TakeHandles(std::move(region));
}

EventuallyReadOnlySharedMemoryRegion& EventuallyReadOnlySharedMemoryRegion::
operator=(EventuallyReadOnlySharedMemoryRegion&& region) {
  std::tie(rw_handle_, ro_handle_) = TakeHandles(std::move(region));
  return *this;
}

EventuallyReadOnlySharedMemoryRegion::~EventuallyReadOnlySharedMemoryRegion() {
  Close();
}

WritableSharedMemoryMapping EventuallyReadOnlySharedMemoryRegion::Map(
    size_t bytes) {
  return MapAt(0, bytes);
}

WritableSharedMemoryMapping EventuallyReadOnlySharedMemoryRegion::MapAt(
    off_t offset,
    size_t bytes) {
  if (!IsValid())
    return {};

  SharedMemory shm(rw_handle_, false /* read_only */);
  bool success = shm.MapAt(offset, bytes);
  size_t mapped_size = shm.mapped_size();
  void* memory = shm.TakeMemory();
  rw_handle_ = shm.TakeHandle();

  if (!success)
    memory = nullptr;

  return WritableSharedMemoryMapping(memory, mapped_size, rw_handle_.GetGUID());
}

bool EventuallyReadOnlySharedMemoryRegion::IsValid() {
  return rw_handle_.IsValid() && ro_handle_.IsValid();
}

void EventuallyReadOnlySharedMemoryRegion::Close() {
  if (rw_handle_.IsValid()) {
    rw_handle_.Close();
    rw_handle_ = SharedMemoryHandle();
  }
  if (ro_handle_.IsValid()) {
    ro_handle_.Close();
    ro_handle_ = SharedMemoryHandle();
  }
}

}  // namespace base
