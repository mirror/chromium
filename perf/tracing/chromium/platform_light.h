// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_TRACING_CORE_CHROMIUM_PLATFORM_LIGHT_H_
#define PERF_TRACING_CORE_CHROMIUM_PLATFORM_LIGHT_H_

// This header is used by the macros headers in tracing/core. Don't add extra
// churn here and prefer using platform_full.h, unless strictly required for
// the tracing macros / inline functions.

#include <stdint.h>

#include "base/atomicops.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"

namespace tracing {
namespace platform {

// Atomics
using AtomicWord = base::subtle::AtomicWord;
using NoBarrier_Load = base::subtle::NoBarrier_Load;
using NoBarrier_Store = base::subtle::NoBarrier_Store;
using Acquire_Load = base::subtle::Acquire_Load;
using Release_Store = base::subtle::Release_Store;

inline int GetCurrentThreadId() {
  return static_cast<int>(base::PlatformThread::CurrentId());
}

// TODO which time unit? signed vs unsigned (For diffs).
inline uint64_t Now() {
  return base::TimeTicks::Now().ToInternalValue();
};

inline uint64_t ThreadNow() {
  return base::ThreadTicks::IsSupported()
             ? base::ThreadTicks::Now().ToInternalValue()
             : 0;
}

}  // namespace platform
}  // namespace tracing

#endif  // PERF_TRACING_CORE_CHROMIUM_PLATFORM_LIGHT_H_
