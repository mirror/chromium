// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/unsafe_shared_memory_region.h"

#include "base/memory/shared_memory.h"

namespace base {

// static
UnsafeSharedMemoryRegion UnsafeSharedMemoryRegion::Create(size_t size) {
  SharedMemoryCreateOptions options;
  options.size = size;
  SharedMemory shm;
  if (!shm.Create(options))
    return {};

  return UnsafeSharedMemoryRegion(shm.TakeHandle());
}

// static
UnsafeSharedMemoryRegion UnsafeSharedMemoryRegion::Deserialize(
    SharedMemoryHandle handle) {
  return UnsafeSharedMemoryRegion(handle);
}

// static
SharedMemoryHandle UnsafeSharedMemoryRegion::TakeHandleForSerialization(
    UnsafeSharedMemoryRegion&& region) {
  SharedMemoryHandle handle_copy = region.handle_;
  region.handle_ = SharedMemoryHandle();
  return handle_copy;
}

UnsafeSharedMemoryRegion::UnsafeSharedMemoryRegion() = default;

UnsafeSharedMemoryRegion::UnsafeSharedMemoryRegion(SharedMemoryHandle handle)
    : handle_(handle) {}

UnsafeSharedMemoryRegion::UnsafeSharedMemoryRegion(
    UnsafeSharedMemoryRegion&& region)
    : handle_(TakeHandleForSerialization(std::move(region))) {}

UnsafeSharedMemoryRegion& UnsafeSharedMemoryRegion::operator=(
    UnsafeSharedMemoryRegion&& region) {
  handle_ = TakeHandleForSerialization(std::move(region));
  return *this;
}

UnsafeSharedMemoryRegion::~UnsafeSharedMemoryRegion() {
  Close();
}

UnsafeSharedMemoryRegion UnsafeSharedMemoryRegion::Duplicate() {
  return UnsafeSharedMemoryRegion(handle_.Duplicate());
}

WritableSharedMemoryMapping UnsafeSharedMemoryRegion::Map(size_t bytes) {
  return MapAt(0, bytes);
}

WritableSharedMemoryMapping UnsafeSharedMemoryRegion::MapAt(off_t offset,
                                                            size_t bytes) {
  if (!IsValid())
    return {};

  SharedMemory shm(handle_, false /* read_only */);
  bool success = shm.MapAt(offset, bytes);
  void* memory = shm.memory();
  shm.TakeHandle();

  if (!success)
    memory = nullptr;

  return WritableSharedMemoryMapping(memory, bytes, handle_.GetGUID());
}

bool UnsafeSharedMemoryRegion::IsValid() {
  return handle_.IsValid();
}

void UnsafeSharedMemoryRegion::Close() {
  if (handle_.IsValid()) {
    handle_.Close();
    handle_ = SharedMemoryHandle();
  }
}

}  // namespace base
