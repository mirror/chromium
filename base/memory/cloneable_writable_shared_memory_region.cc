// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/cloneable_writable_shared_memory_region.h"

#include "base/memory/shared_memory.h"

namespace base {

// static
CloneableWritableSharedMemoryRegion CloneableWritableSharedMemoryRegion::Create(
    size_t size) {
  SharedMemoryCreateOptions options;
  options.size = size;
  SharedMemory shm;
  if (!shm.Create(options))
    return {};

  return CloneableWritableSharedMemoryRegion(shm.TakeHandle());
}

// static
CloneableWritableSharedMemoryRegion
CloneableWritableSharedMemoryRegion::Deserialize(SharedMemoryHandle handle) {
  return CloneableWritableSharedMemoryRegion(handle);
}

// static
SharedMemoryHandle
CloneableWritableSharedMemoryRegion::TakeHandleForSerialization(
    CloneableWritableSharedMemoryRegion&& region) {
  SharedMemoryHandle handle_copy = region.handle_;
  region.handle_ = SharedMemoryHandle();
  return handle_copy;
}

CloneableWritableSharedMemoryRegion::CloneableWritableSharedMemoryRegion() =
    default;

CloneableWritableSharedMemoryRegion::CloneableWritableSharedMemoryRegion(
    SharedMemoryHandle handle)
    : handle_(handle) {}

CloneableWritableSharedMemoryRegion::CloneableWritableSharedMemoryRegion(
    CloneableWritableSharedMemoryRegion&& region)
    : handle_(TakeHandleForSerialization(std::move(region))) {}

CloneableWritableSharedMemoryRegion& CloneableWritableSharedMemoryRegion::
operator=(CloneableWritableSharedMemoryRegion&& region) {
  handle_ = TakeHandleForSerialization(std::move(region));
  return *this;
}

CloneableWritableSharedMemoryRegion::~CloneableWritableSharedMemoryRegion() {
  Close();
}

CloneableWritableSharedMemoryRegion
CloneableWritableSharedMemoryRegion::Clone() {
  return CloneableWritableSharedMemoryRegion(handle_.Duplicate());
}

WritableSharedMemoryMapping CloneableWritableSharedMemoryRegion::Map(
    size_t bytes) {
  return MapAt(0, bytes);
}

WritableSharedMemoryMapping CloneableWritableSharedMemoryRegion::MapAt(
    off_t offset,
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

bool CloneableWritableSharedMemoryRegion::IsValid() {
  return handle_.IsValid();
}

void CloneableWritableSharedMemoryRegion::Close() {
  if (handle_.IsValid()) {
    handle_.Close();
    handle_ = SharedMemoryHandle();
  }
}

}  // namespace base
