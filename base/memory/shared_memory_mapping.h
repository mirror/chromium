// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MEMORY_SHARED_MEMORY_MAPPING_H_
#define BASE_MEMORY_SHARED_MEMORY_MAPPING_H_

#include <cstddef>

#include "base/macros.h"
#include "base/unguessable_token.h"

namespace base {

// Abstract class for a mapped memory region. Not precisely abstract, but not
// useful until the appropriate void* memory() method is exposed by a subclass.
//
// SharedMemoryMapping instance survive the destruction of the parent
// SharedMemoryRegion that created them.
//
// Each instance holds a 128-bit GUID that uniquely identifies the shared
// region it comes from in the system.
class BASE_EXPORT SharedMemoryMapping {
 public:
  // Default constructor initializes an invalid instance.
  SharedMemoryMapping();

  // Move operations are allowed.
  SharedMemoryMapping(SharedMemoryMapping&& mapping);
  SharedMemoryMapping& operator=(SharedMemoryMapping&& mapping);

  // Default destructor. This will call Unmap() if the mapping is valid.
  virtual ~SharedMemoryMapping();

  // Returns true iff the mapping is valid. False means there is no
  // corresponding area of memory.
  bool IsValid() const { return memory_ != nullptr; }

  // Returns the size of the mapping in bytes. This is page-aligned. This is
  // undefined for invalid instances.
  size_t mapped_size() const { return size_; }

  // Returns a const reference to the 128-bit GUID of the region this mapping
  // belongs to.
  const UnguessableToken& mapped_id() const { return guid_; }

  // Unmaps an existing mapping, if needed. The instance becomes invalid after
  // this call. Returns true on success, false on failure (which occurs when
  // trying to unmap an invalid mapping).
  bool Unmap();

  enum { MAP_MINIMUM_ALIGNMENT = 32 };

 protected:
  SharedMemoryMapping(void* address,
                      size_t size,
                      const base::UnguessableToken& guid);
  void* raw_memory_ptr() const { return memory_; }

 private:
  void* memory_ = nullptr;
  size_t size_ = 0;
  base::UnguessableToken guid_;

  DISALLOW_COPY_AND_ASSIGN(SharedMemoryMapping);
};

// Class modelling a read-only mapping of a shared memory region into the
// current process' address space. These should be created by
// ReadOnlySharedMemoryRegion instances.
class BASE_EXPORT ReadOnlySharedMemoryMapping : public SharedMemoryMapping {
 public:
  // Default constructor initializes an invalid instance.
  ReadOnlySharedMemoryMapping();

  // Move operations are allowed.
  ReadOnlySharedMemoryMapping(ReadOnlySharedMemoryMapping&&);
  ReadOnlySharedMemoryMapping& operator=(ReadOnlySharedMemoryMapping&&);

  // Returns the base address of the mapping. This is read-only memory. This is
  // page-aligned. This is nullptr for invalid instances.
  const void* memory() const { return raw_memory_ptr(); }

 private:
  friend class ReadOnlySharedMemoryRegion;
  ReadOnlySharedMemoryMapping(void* address,
                              size_t size,
                              const base::UnguessableToken& guid);

  DISALLOW_COPY_AND_ASSIGN(ReadOnlySharedMemoryMapping);
};

// Class modelling a writable mapping of a shared memory region into the
// current process' address space. These should be created by
// *SharedMemoryRegion instances.
class BASE_EXPORT WritableSharedMemoryMapping : public SharedMemoryMapping {
 public:
  // Default constructor initializes an invalid instance.
  WritableSharedMemoryMapping();

  // Move operations are allowed.
  WritableSharedMemoryMapping(WritableSharedMemoryMapping&&);
  WritableSharedMemoryMapping& operator=(WritableSharedMemoryMapping&&);

  // Returns the base address of the mapping. This is writable memory. This is
  // page-aligned. This is nullptr for invalid instances.
  void* memory() const { return raw_memory_ptr(); }

 private:
  friend class ReadOnlySharedMemoryRegion;
  friend class WritableSharedMemoryRegion;
  friend class EventuallyReadOnlySharedMemoryRegion;
  WritableSharedMemoryMapping(void* address,
                              size_t size,
                              const base::UnguessableToken& guid);

  DISALLOW_COPY_AND_ASSIGN(WritableSharedMemoryMapping);
};

}  // namespace base

#endif  // BASE_MEMORY_SHARED_MEMORY_MAPPING_H_
