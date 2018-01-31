// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/writable_shared_memory_region.h"

#include "base/memory/shared_memory.h"

namespace base {

// static
WritableSharedMemoryRegion WritableSharedMemoryRegion::Create(size_t size) {
  SharedMemoryCreateOptions options;
  options.size = size;
  SharedMemory shm;
  if (!shm.Create(options))
    return {};

  return WritableSharedMemoryRegion(shm.TakeHandle());
}

// static
SharedMemoryHandle WritableSharedMemoryRegion::TakeHandle(
    WritableSharedMemoryRegion&& region) {
  SharedMemoryHandle handle_copy = region.handle_;
  region.handle_ = SharedMemoryHandle();
  return handle_copy;
}

WritableSharedMemoryRegion::WritableSharedMemoryRegion() = default;

WritableSharedMemoryRegion::WritableSharedMemoryRegion(
    SharedMemoryHandle handle)
    : handle_(handle) {}

WritableSharedMemoryRegion::WritableSharedMemoryRegion(
    WritableSharedMemoryRegion&& region)
    : handle_(TakeHandle(std::move(region))) {}

WritableSharedMemoryRegion& WritableSharedMemoryRegion::operator=(
    WritableSharedMemoryRegion&& region) {
  handle_ = TakeHandle(std::move(region));
  return *this;
}

WritableSharedMemoryRegion::~WritableSharedMemoryRegion() {
  Close();
}

WritableSharedMemoryRegion WritableSharedMemoryRegion::Duplicate() {
  return WritableSharedMemoryRegion(handle_.Duplicate());
}

WritableSharedMemoryMapping WritableSharedMemoryRegion::Map(size_t bytes) {
  return MapAt(0, bytes);
}

WritableSharedMemoryMapping WritableSharedMemoryRegion::MapAt(off_t offset,
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

bool WritableSharedMemoryRegion::IsValid() {
  return handle_.IsValid();
}

void WritableSharedMemoryRegion::Close() {
  if (handle_.IsValid()) {
    handle_.Close();
    handle_ = SharedMemoryHandle();
  }
}

}  // namespace base
