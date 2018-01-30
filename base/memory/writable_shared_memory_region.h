// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MEMORY_WRITABLE_SHARED_MEMORY_REGION_H_
#define BASE_MEMORY_WRITABLE_SHARED_MEMORY_REGION_H_

#include "base/macros.h"
#include "base/memory/shared_memory_handle.h"
#include "base/memory/shared_memory_mapping.h"

namespace base {

class BASE_EXPORT WritableSharedMemoryRegion {
 public:
  static WritableSharedMemoryRegion Create(size_t size);

  static SharedMemoryHandle TakeHandle(WritableSharedMemoryRegion&& region);

  WritableSharedMemoryRegion();
  WritableSharedMemoryRegion(SharedMemoryHandle handle);

  // Move operations are allowed.
  WritableSharedMemoryRegion(WritableSharedMemoryRegion&&);
  WritableSharedMemoryRegion& operator=(WritableSharedMemoryRegion&&);

  ~WritableSharedMemoryRegion();

  WritableSharedMemoryRegion Duplicate();

  WritableSharedMemoryMapping Map(size_t bytes);

  WritableSharedMemoryMapping MapAt(off_t offset, size_t bytes);

  bool IsValid();

  void Close();

 private:
  SharedMemoryHandle handle_;

  DISALLOW_COPY_AND_ASSIGN(WritableSharedMemoryRegion);
};

}  // namespace base

#endif  // BASE_MEMORY_WRITABLE_SHARED_MEMORY_REGION_H_
