// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MEMORY_EVENTUALLY_READ_ONLY_SHARED_MEMORY_REGION_H_
#define BASE_MEMORY_EVENTUALLY_READ_ONLY_SHARED_MEMORY_REGION_H_

#include "base/macros.h"
#include "base/memory/read_only_shared_memory_region.h"
#include "base/memory/shared_memory_handle.h"
#include "base/memory/shared_memory_mapping.h"

namespace base {

// Class modelling platform shared memory with write access that can be later
// demoted to read-only.
// EventuallyReadOnlySharedMemoryRegion wraps a pair of platform-specific
// shared memory region handles/descriptors.
//
// This class allows to map a shared memory OS resource into the virtual
// address space of the current process. All generated mappings are writable
// and survive the destruction of this class.
//
// Use this class only if you need to have writable mappings of the same region
// in several processes AND this region should be shared with other processes as
// read-only.
//
// EventuallyReadOnlySharedMemoryRegion cannot be duplicated in the current
// process but can be passed to another process that will become an exclusive
// owner of the region.
//
// EventuallyReadOnlySharedMemoryRegion can be eventually converted to the
// ReadOnlySharedMemoryRegion losing the ability to create new writable
// mappings of this region.
class BASE_EXPORT EventuallyReadOnlySharedMemoryRegion {
 public:
  // Creates a new EventuallyReadOnlySharedMemoryRegion instance of a given
  // size that can be used for mapping writable shared memory into the virtual
  // address space.
  static EventuallyReadOnlySharedMemoryRegion Create(size_t size);

  // Extracts a pair of platform handles from the region. The caller gets
  // ownership of the handles. This should be used only for sending the handles
  // from the current process to another.
  static std::pair<SharedMemoryHandle, SharedMemoryHandle> TakeHandles(
      EventuallyReadOnlySharedMemoryRegion&& region);

  // Makes the region read-only. No new writable mappings of the region can be
  // created after this call.
  static ReadOnlySharedMemoryRegion ConvertToReadOnly(
      EventuallyReadOnlySharedMemoryRegion&&);

  // Default constructor initializes an invalid instance.
  EventuallyReadOnlySharedMemoryRegion();

  // Constructs EventuallyReadOnlySharedMemoryRegion from a pair of
  // platform-specific handles as they were taken from another
  // EventuallyReadOnlySharedMemoryRegion instance.
  // This should be used only by the code passing handles across process
  // boundaries.
  EventuallyReadOnlySharedMemoryRegion(SharedMemoryHandle rw_handle,
                                       SharedMemoryHandle ro_handle);

  // Move operations are allowed.
  EventuallyReadOnlySharedMemoryRegion(EventuallyReadOnlySharedMemoryRegion&&);
  EventuallyReadOnlySharedMemoryRegion& operator=(
      EventuallyReadOnlySharedMemoryRegion&&);

  // Destructor will call Close() if underlying platform handles are valid. All
  // created mappings will remain valid.
  ~EventuallyReadOnlySharedMemoryRegion();

  // Maps the shared memory into the caller's address space with write access.
  // Returns a valid WritableSharedMemoryMapping instance on success, unvalid
  // otherwise. The mapped address is guaranteed to have an alignment of at
  // least MAP_MINIMUM_ALIGNMENT.
  WritableSharedMemoryMapping Map(size_t bytes);

  // Same as above, but with |offset| to specify from beginning of the shared
  // memory block to map.
  // |offset| must be aligned to value of |SysInfo::VMAllocationGranularity()|.
  WritableSharedMemoryMapping MapAt(off_t offset, size_t bytes);

  // Whether underlying platform handles are valid.
  bool IsValid();

  // Closes underlying platform handles if needed. The instance becomes invalid
  // after this call. All created mappings will remain valid.
  void Close();

 private:
  SharedMemoryHandle rw_handle_;
  SharedMemoryHandle ro_handle_;

  DISALLOW_COPY_AND_ASSIGN(EventuallyReadOnlySharedMemoryRegion);
};

}  // namespace base

#endif  // BASE_MEMORY_EVENTUALLY_READ_ONLY_SHARED_MEMORY_REGION_H_
