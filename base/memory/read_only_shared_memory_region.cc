// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/read_only_shared_memory_region.h"

#include "base/memory/shared_memory.h"
#include "build/build_config.h"

namespace base {

// static
std::pair<ReadOnlySharedMemoryRegion, WritableSharedMemoryMapping>
ReadOnlySharedMemoryRegion::Create(size_t size) {
  SharedMemoryCreateOptions options;
  options.size = size;
  options.share_read_only = true;
  SharedMemory shm;
  if (!shm.Create(options))
    return {};

  ReadOnlySharedMemoryRegion region(shm.GetReadOnlyHandle());
  if (!region.IsValid())
    return {};

  if (!shm.Map(size))
    return {};

  size_t mapped_size = shm.mapped_size();
  WritableSharedMemoryMapping mapping(shm.TakeMemory(), mapped_size,
                                      shm.handle().GetGUID());
  SharedMemoryHandle writable_handle = shm.TakeHandle();
  if (writable_handle.IsValid())
    writable_handle.Close();

#if defined(OS_ANDROID)
  if (!region.handle_.SetRegionReadOnly())
    return {};
#endif  // defined(OS_ANDROID)

  return {std::move(region), std::move(mapping)};
}

// static
SharedMemoryHandle ReadOnlySharedMemoryRegion::TakeHandle(
    ReadOnlySharedMemoryRegion&& region) {
  SharedMemoryHandle handle_copy = region.handle_;
  region.handle_ = SharedMemoryHandle();
  return handle_copy;
}

ReadOnlySharedMemoryRegion::ReadOnlySharedMemoryRegion() = default;

ReadOnlySharedMemoryRegion::ReadOnlySharedMemoryRegion(
    SharedMemoryHandle handle)
    : handle_(handle) {}

ReadOnlySharedMemoryRegion::ReadOnlySharedMemoryRegion(
    ReadOnlySharedMemoryRegion&& region)
    : handle_(TakeHandle(std::move(region))) {}

ReadOnlySharedMemoryRegion& ReadOnlySharedMemoryRegion::operator=(
    ReadOnlySharedMemoryRegion&& region) {
  handle_ = TakeHandle(std::move(region));
  return *this;
}

ReadOnlySharedMemoryRegion::~ReadOnlySharedMemoryRegion() {
  Close();
}

ReadOnlySharedMemoryRegion ReadOnlySharedMemoryRegion::Duplicate() {
  return ReadOnlySharedMemoryRegion(handle_.Duplicate());
}

ReadOnlySharedMemoryMapping ReadOnlySharedMemoryRegion::Map(size_t bytes) {
  return MapAt(0, bytes);
}

ReadOnlySharedMemoryMapping ReadOnlySharedMemoryRegion::MapAt(off_t offset,
                                                              size_t bytes) {
  if (!IsValid())
    return {};

  SharedMemory shm(handle_, true /* read_only */);
  bool success = shm.MapAt(offset, bytes);
  size_t mapped_size = shm.mapped_size();
  void* memory = shm.TakeMemory();
  handle_ = shm.TakeHandle();

  if (!success)
    memory = nullptr;

  return ReadOnlySharedMemoryMapping(memory, mapped_size, handle_.GetGUID());
}

bool ReadOnlySharedMemoryRegion::IsValid() {
  return handle_.IsValid();
}

void ReadOnlySharedMemoryRegion::Close() {
  if (handle_.IsValid()) {
    handle_.Close();
    handle_ = SharedMemoryHandle();
  }
}

}  // namespace base
