// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_TRACING_CORE_IMPL_AUTO_LOCK_H_
#define PERF_TRACING_CORE_IMPL_AUTO_LOCK_H_

#include "perf/tracing/core/common/macros.h"
#include "perf/tracing/platform.h"

namespace tracing {
namespace internal {

class AutoLock {
 public:
  struct AlreadyAcquired {};

  explicit AutoLock(::tracing::platform::Lock& lock) : lock_(lock) {
    lock_.Acquire();
  }

  AutoLock(::tracing::platform::Lock& lock, const AlreadyAcquired&)
      : lock_(lock) {
    lock_.AssertAcquired();
  }

  ~AutoLock() {
    lock_.AssertAcquired();
    lock_.Release();
  }

 private:
  TRACING_DISALLOW_COPY_AND_ASSIGN(AutoLock);
};

}  // namespace internal
}  // namespace tracing

#endif  // PERF_TRACING_CORE_IMPL_AUTO_LOCK_H_
