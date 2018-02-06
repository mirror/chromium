// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/shared_memory_mapping.h"

#include "base/logging.h"
#include "base/memory/shared_memory_tracker.h"
#include "base/unguessable_token.h"
#include "build/build_config.h"

#if defined(OS_POSIX)
#include <sys/mman.h>
#endif

#if defined(OS_WIN)
#include <aclapi.h>
#endif

#if defined(OS_MACOSX) && !defined(OS_IOS)
#include <mach/mach_vm.h>
#endif

#if defined(OS_FUCHSIA)
#include <zircon/process.h>
#include <zircon/syscalls.h>
#endif

namespace base {

SharedMemoryMapping::SharedMemoryMapping() = default;

SharedMemoryMapping::SharedMemoryMapping(SharedMemoryMapping&& mapping) {
  using std::swap;
  swap(memory_, mapping.memory_);
  swap(size_, mapping.size_);
  swap(guid_, mapping.guid_);
}

SharedMemoryMapping& SharedMemoryMapping::operator=(
    SharedMemoryMapping&& mapping) {
  using std::swap;
  swap(memory_, mapping.memory_);
  swap(size_, mapping.size_);
  swap(guid_, mapping.guid_);
  return *this;
}

SharedMemoryMapping::SharedMemoryMapping(void* memory,
                                         size_t size,
                                         const UnguessableToken& guid)
    : memory_(memory), size_(size), guid_(guid) {
  //  SharedMemoryTracker::GetInstance()->IncrementMemoryUsage(
  //      memory_, size_, guid_);
}

SharedMemoryMapping::~SharedMemoryMapping() {
  Unmap();
}

bool SharedMemoryMapping::Unmap() {
  if (!IsValid())
    return false;

#if defined(OS_WIN)
  if (!UnmapViewOfFile(memory_)) {
    DLOG(ERROR) << "UnmapViewOfFile error=" << GetLastError();
    return false;
  }
#elif defined(OS_FUCHSIA)
  uintptr_t addr = reinterpret_cast<uintptr_t>(memory_);
  zx_status_t status = zx_vmar_unmap(zx_vmar_root_self(), addr, size_);
  if (status != ZX_OK) {
    DLOG(ERROR) << "zx_vmar_unmap failed, status=" << status;
    return false;
  }
#elif defined(OS_MACOSX) && !defined(OS_IOS)
  kern_return_t ret = mach_vm_deallocate(
      mach_task_self(), reinterpret_cast<mach_vm_address_t>(memory_), size_);
  if (ret != 0) {
    DPLOG(ERROR) << "mach_vm_deallocate error=" << ret;
    return false;
  }
#else
  if (munmap(memory_, size_) < 0) {
    DPLOG(ERROR) << "munmap";
    return false;
  }
#endif
  // SharedMemoryTracker::GetInstance()->DecrementMemoryUsage(memory_);

  memory_ = nullptr;
  size_ = 0;
  guid_ = UnguessableToken();
  return true;
}

ReadOnlySharedMemoryMapping::ReadOnlySharedMemoryMapping() = default;
ReadOnlySharedMemoryMapping::ReadOnlySharedMemoryMapping(
    ReadOnlySharedMemoryMapping&&) = default;
ReadOnlySharedMemoryMapping& ReadOnlySharedMemoryMapping::operator=(
    ReadOnlySharedMemoryMapping&&) = default;
ReadOnlySharedMemoryMapping::ReadOnlySharedMemoryMapping(
    void* address,
    size_t size,
    const base::UnguessableToken& guid)
    : SharedMemoryMapping(address, size, guid) {}

WritableSharedMemoryMapping::WritableSharedMemoryMapping() = default;
WritableSharedMemoryMapping::WritableSharedMemoryMapping(
    WritableSharedMemoryMapping&&) = default;
WritableSharedMemoryMapping& WritableSharedMemoryMapping::operator=(
    WritableSharedMemoryMapping&&) = default;
WritableSharedMemoryMapping::WritableSharedMemoryMapping(
    void* address,
    size_t size,
    const base::UnguessableToken& guid)
    : SharedMemoryMapping(address, size, guid) {}

}  // namespace base
