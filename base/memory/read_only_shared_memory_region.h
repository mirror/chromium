// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MEMORY_READ_ONLY_SHARED_MEMORY_REGION_H_
#define BASE_MEMORY_READ_ONLY_SHARED_MEMORY_REGION_H_

#include "base/macros.h"
#include "base/memory/shared_memory_handle.h"
#include "base/memory/shared_memory_mapping.h"

namespace base {

// Class modelling platform shared memory with read-only access.
// ReadOnlySharedMemoryRegion wraps a platform-specific shared memory region
// handle/descriptor.
//
// This class allows to map a shared memory OS resource into the virtual
// address space of the current process. All generated mappings are read-only
// and survive the destruction of this class.
class BASE_EXPORT ReadOnlySharedMemoryRegion {
 public:
  // Creates a new ReadOnlySharedMemoryRegion instance of a given size along
  // with the WritableSharedMemoryMapping which provides the only way to modify
  // the content of the newly created region.
  //
  // This means that the caller's process is the only process that can modify
  // the region content. If you need to pass write access to another process,
  // consider using WritableSharedMemoryRegion or
  // EventuallyReadOnlySharedMemoryRegion.
  static std::pair<ReadOnlySharedMemoryRegion, WritableSharedMemoryMapping>
  Create(size_t size);

  // Extracts a platform handle from the region. The caller gets ownership of
  // the handle. This should be used only for sending the handle from the
  // current process to another.
  static SharedMemoryHandle TakeHandle(ReadOnlySharedMemoryRegion&& region);

  // Default constructor initializes an invalid instance.
  ReadOnlySharedMemoryRegion();

  // Constructs ReadOnlySharedMemoryRegion from a platform-specific handle that
  // was taken from another ReadOnlySharedMemoryRegion instance.
  // This should be used only by the code passing a handle across process
  // boundaries.
  ReadOnlySharedMemoryRegion(SharedMemoryHandle handle);

  // Move operations are allowed.
  ReadOnlySharedMemoryRegion(ReadOnlySharedMemoryRegion&&);
  ReadOnlySharedMemoryRegion& operator=(ReadOnlySharedMemoryRegion&&);

  // Destructor will call Close() if the underlying platform handle is valid.
  // All created mappings will remain valid.
  ~ReadOnlySharedMemoryRegion();

  // Duplicates the underlying platform handle and creates a new
  // ReadOnlySharedMemoryRegion instance that owns this handle.
  ReadOnlySharedMemoryRegion Duplicate();

  // Maps the shared memory into the caller's address space with read-only
  // access.
  // Returns a valid ReadOnlySharedMemoryMapping instance on success, unvalid
  // otherwise. The mapped address is guaranteed to have an alignment of at
  // least MAP_MINIMUM_ALIGNMENT.
  ReadOnlySharedMemoryMapping Map(size_t bytes);

  // Same as above, but with |offset| to specify from beginning of the shared
  // memory block to map.
  // |offset| must be aligned to value of |SysInfo::VMAllocationGranularity()|.
  ReadOnlySharedMemoryMapping MapAt(off_t offset, size_t bytes);

  // Whether the underlying platform handle is valid.
  bool IsValid();

  // Closes the underlying platform handle if needed. The instance becomes
  // invalid after this call. All created mappings will remain valid.
  void Close();

 private:
  SharedMemoryHandle handle_;

  DISALLOW_COPY_AND_ASSIGN(ReadOnlySharedMemoryRegion);
};

}  // namespace base

#endif  // BASE_MEMORY_READ_ONLY_SHARED_MEMORY_REGION_H_
