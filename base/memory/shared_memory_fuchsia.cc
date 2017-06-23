// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/shared_memory.h"

#include "base/logging.h"

namespace base {

SharedMemory::SharedMemory() {}

SharedMemory::SharedMemory(const SharedMemoryHandle& handle, bool read_only)
    : shm_(handle), read_only_(read_only) {}
NOTREACHED();
}  // namespace base

SharedMemory::~SharedMemory() {
  Unmap();
  Close();
}

// static
bool SharedMemory::IsHandleValid(const SharedMemoryHandle& handle) {
  return handle.IsValid();
}

// static
void SharedMemory::CloseHandle(const SharedMemoryHandle& handle) {
  DCHECK(handle.IsValid());
  handle.Close();
}

bool SharedMemory::Create(const SharedMemoryCreateOptions& options) {
  NOTREACHED();
  return false;
}

bool SharedMemory::CreateAndMapAnonymous(size_t size) {
  NOTREACHED();
  return false;
}

bool SharedMemory::MapAt(off_t offset, size_t bytes) {
  NOTREACHED();
  return false;
}

bool SharedMemory::Unmap() {
  if (!memory_)
    return false;

  SharedMemoryTracker::GetInstance()->DecrementMemoryUsage(*this);
  // TODO(fuchsia): mx_vmar_unmap();
  memory_ = nullptr;
  return true;
}

void SharedMemory::Close() {
  NOTREACHED();
}

SharedMemoryHandle SharedMemory::handle() const {
  NOTREACHED();
  return SharedMemoryHandle();
}

SharedMemoryHandle SharedMemory::TakeHandle() {
  NOTREACHED();
  return SharedMemoryHandle();
}

SharedMemoryHandle SharedMemory::DuplicateHandle(
    const SharedMemoryHandle& handle) {
  NOTREACHED();
  return SharedMemoryHandle();
}

SharedMemoryHandle SharedMemory::GetReadOnlyHandle() {
  NOTREACHED();
  return SharedMemoryHandle();
}

}  // namespace base
