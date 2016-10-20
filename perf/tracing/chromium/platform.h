// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_TRACING_CORE_CHROMIUM_PLATFORM_H_
#define PERF_TRACING_CORE_CHROMIUM_PLATFORM_H_

#include "base/threading/platform_thread.h"
#include "base/time/time.h"

namespace tracing {
namespace platform {

inline int GetCurrentThreadId() {
  return static_cast<int>(base::PlatformThread::CurrentId());
}

// TODO which time unit? signed vs unsigned (For diffs).
inline uint64_t GetWallTime() {
  return base::TimeTicks::Now().ToInternalValue();
};

inline uint64_t GetThreadTime() {
  return base::ThreadTicks::IsSupported()
             ? base::ThreadTicks::Now().ToInternalValue()
             : 0;
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
}

#endif  // PERF_TRACING_CORE_CHROMIUM_PLATFORM_H_
