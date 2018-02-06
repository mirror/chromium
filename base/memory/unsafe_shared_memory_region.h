// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MEMORY_UNSAFE_SHARED_MEMORY_REGION_H_
#define BASE_MEMORY_UNSAFE_SHARED_MEMORY_REGION_H_

#include "base/macros.h"
#include "base/memory/shared_memory_handle.h"
#include "base/memory/shared_memory_mapping.h"

namespace base {

// Class modelling platform shared memory with write access.
// UnsafeSharedMemoryRegion wraps a platform-specific shared memory
// region handle/descriptor.
//
// This class allows to map a shared memory OS resource into the virtual
// address space of the current process. All generated mappings are writable
// and survive the destruction of this class.
//
// UnsafeSharedMemoryRegion cannot be converted to read-only, so
// every process getting a handle to this region will have write access.
class BASE_EXPORT UnsafeSharedMemoryRegion {
 public:
  // Creates a new UnsafeSharedMemoryRegion instance of a given size that can be
  // used for mapping writable shared memory into the virtual address space.
  static UnsafeSharedMemoryRegion Create(size_t size);

  // Returns UnsafeSharedMemoryRegion built from a platform-specific handle that
  // was taken from another UnsafeSharedMemoryRegion instance. This should be
  // used only by the code passing a handle across process boundaries.
  static UnsafeSharedMemoryRegion Deserialize(SharedMemoryHandle handle);

  // Extracts a platform handle from the region. The caller gets ownership of
  // the handle. This should be used only for sending the handle from the
  // current process to another.
  static SharedMemoryHandle TakeHandleForSerialization(
      UnsafeSharedMemoryRegion&& region);

  // Default constructor initializes an invalid instance.
  UnsafeSharedMemoryRegion();

  // Move operations are allowed.
  UnsafeSharedMemoryRegion(UnsafeSharedMemoryRegion&&);
  UnsafeSharedMemoryRegion& operator=(UnsafeSharedMemoryRegion&&);

  // Destructor will call Close() if the underlying platform handle is valid.
  // All created mappings will remain valid.
  ~UnsafeSharedMemoryRegion();

  // Duplicates the underlying platform handle and creates a new
  // UnsafeSharedMemoryRegion instance that owns this handle.
  UnsafeSharedMemoryRegion Duplicate();

  // Maps the shared memory into the caller's address space with write access.
  // Returns a valid WritableSharedMemoryMapping instance on success, unvalid
  // otherwise. The mapped address is guaranteed to have an alignment of at
  // least MAP_MINIMUM_ALIGNMENT.
  WritableSharedMemoryMapping Map(size_t bytes);

  // Same as above, but with |offset| to specify from beginning of the shared
  // memory block to map.
  // |offset| must be aligned to value of |SysInfo::VMAllocationGranularity()|.
  WritableSharedMemoryMapping MapAt(off_t offset, size_t bytes);

  // Whether the underlying platform handle is valid.
  bool IsValid();

  // Closes the underlying platform handle if needed. The instance becomes
  // invalid after this call. All created mappings will remain valid.
  void Close();

 private:
  UnsafeSharedMemoryRegion(SharedMemoryHandle handle);

  SharedMemoryHandle handle_;

  DISALLOW_COPY_AND_ASSIGN(UnsafeSharedMemoryRegion);
};

}  // namespace base

#endif  // BASE_MEMORY_UNSAFE_SHARED_MEMORY_REGION_H_
