// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_TRACING_CORE_CHROMIUM_PLATFORM_H_
#define PERF_TRACING_CORE_CHROMIUM_PLATFORM_H_

#include <stdint.h>

#include "base/atomicops.h"
#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "perf/tracing/core/public/trace_event_handle.h"
#include "perf/tracing/chromium/public/trace_log.h"

namespace tracing {

class ConvertableToTraceFormat;

namespace platform {

// Atomics
using AtomicWord = base::subtle::AtomicWord;
using NoBarrier_Load = base::subtle::NoBarrier_Load;
using NoBarrier_Store = base::subtle::NoBarrier_Store;
using Acquire_Load = base::subtle::Acquire_Load;
using Release_Store = base::subtle::Release_Store;

inline TraceEventHandle AddTraceEvent(
    char phase,
    const unsigned char* category_group_enabled,
    const char* name,
    const char* scope,
    uint64_t id,
    uint64_t bind_id,
    int thread_id,
    const TimeTicks& timestamp,
    int num_args,
    const char** arg_names,
    const unsigned char* arg_types,
    const uint64_t* arg_values,
    std::unique_ptr<ConvertableToTraceFormat>* convertable_values,
    uint32_t flags) {
  return TraceLog::GetInstance()->AddTraceEventWithThreadIdAndTimestamp(
      phase, category_group_enabled, name, scope, id, bind_id, thread_id,
      timestamp, num_args, arg_names, arg_types, arg_values, convertable_values,
      flags);
}

// TODO what is the unit we are expecting? shouldn't use ToInternalValue().
inline uint64_t Now() {
  return base::TimeTicks::Now().ToInternalValue();
};

inline uint64_t ThreadNow() {
  return base::ThreadTicks::IsSupported()
             ? base::ThreadTicks::Now().ToInternalValue()
             : 0;
}

// Threading & synchronization
inline int GetCurrentThreadId() {
  return static_cast<int>(base::PlatformThread::CurrentId());
}

class Lock {
 public:
  void Acquire() { lock_.Acquire(); }
  void Release() { lock_.Release(); }
  void AssertAcquired() { lock_.AssertAcquired(); }

 private:
  base::Lock lock_;

  DISALLOW_COPY_AND_ASSIGN(Lock);
};

}  // namespace platform
}  // namespace tracing

#endif  // PERF_TRACING_CORE_CHROMIUM_PLATFORM_H_
