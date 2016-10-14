// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PERF_TRACING_CORE_CHROMIUM_PLATFORM_FULL_H_
#define PERF_TRACING_CORE_CHROMIUM_PLATFORM_FULL_H_

// This header is used by the internal implementation of tracing/core.

#include "base/macros.h"
#include "base/synchronization/lock.h"

namespace tracing {
namespace platform {

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

#endif  // PERF_TRACING_CORE_CHROMIUM_PLATFORM_FULL_H_
