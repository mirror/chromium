// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/trace_event/heap_profiler_allocation_crasher.h"

#include "base/debug/dump_without_crashing.h"

#include "base/debug/stack_trace.h"

namespace base {
namespace trace_event {

// static
subtle::Atomic32 HeapProfilerAllocationCrasher::is_enabled_for_allocator_
    [ALLOCATOR_TYPE_LAST] = {0};

HeapProfilerAllocationCrasher::HeapProfilerAllocationCrasher() {}
HeapProfilerAllocationCrasher::~HeapProfilerAllocationCrasher() {}

// static
HeapProfilerAllocationCrasher* HeapProfilerAllocationCrasher::GetInstance() {
  static HeapProfilerAllocationCrasher* instance =
      new HeapProfilerAllocationCrasher();
  return instance;
}

void HeapProfilerAllocationCrasher::EnableCrashingForAllocatorIfPossible(
    HeapProfilerAllocationCrasher::AllocatorType type) {
  AutoLock lock(should_upload_crash_dump_lock_);
  //|should_upload_crash_dump_| is never be set to true to ensure that we upload
  // crash dump only once per process. So, enable checking for upload condition
  // only if it's set to true.
  if (!should_upload_crash_dump_)
    return;
  subtle::Release_Store(&is_enabled_for_allocator_[type], 1);
}

void HeapProfilerAllocationCrasher::UploadCrashReport(
    HeapProfilerAllocationCrasher::AllocatorType type) {
  if (!is_enabled_for_allocator(type))
    return;

  bool should_upload_crash = false;
  {
    AutoLock lock(should_upload_crash_dump_lock_);
    if (should_upload_crash_dump_)
      should_upload_crash = true;
    // Disable uploads when first upload is triggered.
    should_upload_crash_dump_ = false;
  }
  // Disable upload checks for all allocators.
  base::subtle::Release_Store(&is_enabled_for_allocator_[ALLOCATOR_TYPE_MALLOC],
                              0);

  if (should_upload_crash)
    base::debug::DumpWithoutCrashing();
}

}  // namespace trace_event
}  // namespace base
