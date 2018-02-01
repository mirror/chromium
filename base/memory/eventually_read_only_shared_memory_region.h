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

class BASE_EXPORT EventuallyReadOnlySharedMemoryRegion {
 public:
  static EventuallyReadOnlySharedMemoryRegion Create(size_t size);

  static std::pair<SharedMemoryHandle, SharedMemoryHandle> TakeHandles(
      EventuallyReadOnlySharedMemoryRegion&& region);

  static ReadOnlySharedMemoryRegion ConvertToReadOnly(
      EventuallyReadOnlySharedMemoryRegion&&);

  EventuallyReadOnlySharedMemoryRegion();
  EventuallyReadOnlySharedMemoryRegion(SharedMemoryHandle rw_handle,
                                       SharedMemoryHandle ro_handle);

  // Move operations are allowed.
  EventuallyReadOnlySharedMemoryRegion(EventuallyReadOnlySharedMemoryRegion&&);
  EventuallyReadOnlySharedMemoryRegion& operator=(
      EventuallyReadOnlySharedMemoryRegion&&);

  ~EventuallyReadOnlySharedMemoryRegion();

  WritableSharedMemoryMapping Map(size_t bytes);

  WritableSharedMemoryMapping MapAt(off_t offset, size_t bytes);

  bool IsValid();

  void Close();

 private:
  SharedMemoryHandle rw_handle_;
  SharedMemoryHandle ro_handle_;

  DISALLOW_COPY_AND_ASSIGN(EventuallyReadOnlySharedMemoryRegion);
};

}  // namespace base

#endif  // BASE_MEMORY_EVENTUALLY_READ_ONLY_SHARED_MEMORY_REGION_H_
