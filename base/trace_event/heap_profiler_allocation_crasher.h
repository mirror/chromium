// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TRACE_EVENT_HEAP_PROFILER_ALLOCATION_CRASHER_H_
#define BASE_TRACE_EVENT_HEAP_PROFILER_ALLOCATION_CRASHER_H_

#include "base/atomicops.h"
#include "base/synchronization/lock.h"

namespace base {
namespace trace_event {

// Uploads a crash report without actually causing a crash when heap profiler
// detects large number of untracker allocations.
class HeapProfilerAllocationCrasher {
 public:
  enum AllocatorType { ALLOCATOR_TYPE_MALLOC = 0, ALLOCATOR_TYPE_LAST };

  static HeapProfilerAllocationCrasher* GetInstance();

  // This is called in fast allocation path.
  static bool is_enabled_for_allocator(AllocatorType type) {
    return !!subtle::Acquire_Load(&is_enabled_for_allocator_[type]);
  }

  // Enable crash at the next allocation with unknown context. Can be enabled
  // only once in the process and will be disabled permanently once
  // UploadCrashReport() is called by any of the allocator that enabled
  // crashing.
  void EnableCrashingForAllocatorIfPossible(AllocatorType type);

  // Uploads a crash report and disable checks.
  void UploadCrashReport(AllocatorType);

 private:
  HeapProfilerAllocationCrasher();
  ~HeapProfilerAllocationCrasher();

  static subtle::Atomic32 is_enabled_for_allocator_[ALLOCATOR_TYPE_LAST];
  bool should_upload_crash_dump_ = true;
  Lock should_upload_crash_dump_lock_;
};

}  // namespace trace_event
}  // namespace base

#endif  // BASE_TRACE_EVENT_HEAP_PROFILER_ALLOCATION_CRASHER_H_
