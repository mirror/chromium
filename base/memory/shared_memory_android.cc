// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/shared_memory.h"

#include <fcntl.h>
#include <stddef.h>
#include <sys/mman.h>

#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/strings/string_number_conversions.h"
#include "third_party/ashmem/ashmem.h"

namespace base {

// For Android, we use ashmem to implement SharedMemory. ashmem_create_region
// will automatically pin the region. We never explicitly call pin/unpin. When
// all the file descriptors from different processes associated with the region
// are closed, the memory buffer will go away.

bool SharedMemory::Create(const SharedMemoryCreateOptions& options) {
  DCHECK(!shm_.IsValid());

  if (options.size > static_cast<size_t>(std::numeric_limits<int>::max()))
    return false;

  // "name" is just a label in ashmem. It is visible in /proc/pid/maps.
  int fd = ashmem_create_region(
      options.name_deprecated ? options.name_deprecated->c_str() : "",
      options.size);
  shm_ = SharedMemoryHandle::ImportHandle(fd, options.size);
  if (!shm_.IsValid()) {
    DLOG(ERROR) << "Shared memory creation failed";
    return false;
  }

  int flags = PROT_READ | PROT_WRITE | (options.executable ? PROT_EXEC : 0);
  int err = ashmem_set_prot_region(shm_.GetHandle(), flags);
  if (err < 0) {
    DLOG(ERROR) << "Error " << err << " when setting protection of ashmem";
    return false;
  }

  // TECHNICAL NOTE: Ashmem regions have their own read/write/exec mode
  // that is used when mmap() is called. The access mode of Ashmem file
  // descriptors is entirely ignored.
  //
  // What we do here is create a secondary file descriptor that has read-only
  // access mode. Later this is tested by the method
  // SharedMemoryHandle::EnforceReadOnlyRegionIfNeeded(), which will
  // set the region's mask to read-only when the handle is read-only.

  // It is not possible to open a new descriptor to the same region with
  // only read-only access. Also, opening /proc/self/fd/<fd> with O_RDONLY
  // actually works, but the resulting descriptor returns EINVAL errors
  // if used for Ashmem operations.
  //
  // We route around this by using the following *ugly* tVrick:
  int ro_fd = HANDLE_EINTR(dup(fd));
  if (ro_fd < 0) {
    DPLOG(ERROR) << "When creating read-only handle descriptor";
    return false;
  }
  readonly_shm_ = SharedMemoryHandle(base::FileDescriptor(ro_fd, true),
                                     options.size, shm_.GetGUID());

  readonly_shm_.SetReadOnly();

  DCHECK(readonly_shm_.IsReadOnly());
  DCHECK(!shm_.IsReadOnly());

  requested_size_ = options.size;

  return true;
}

bool SharedMemory::Delete(const std::string& name) {
  // Like on Windows, this is intentionally returning true as ashmem will
  // automatically releases the resource when all FDs on it are closed.
  return true;
}

bool SharedMemory::Open(const std::string& name, bool read_only) {
  // ashmem doesn't support name mapping
  NOTIMPLEMENTED();
  return false;
}

}  // namespace base
