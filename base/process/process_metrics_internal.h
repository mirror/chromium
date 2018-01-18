// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains private internal declarations routines for gathering
// resource statistics for processes running on the system. It is a separate
// file so that users of process metrics don't need to include windows.h.

#ifndef BASE_PROCESS_PROCESS_METRICS_INTERNAL_H_
#define BASE_PROCESS_PROCESS_METRICS_INTERNAL_H_

#include <stdint.h>

#include "base/process/process_metrics.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

namespace base {

#if defined(OS_WIN)
struct IoCounters : public IO_COUNTERS {};
#elif defined(OS_POSIX)
struct IoCounters {
  uint64_t ReadOperationCount;
  uint64_t WriteOperationCount;
  uint64_t OtherOperationCount;
  uint64_t ReadTransferCount;
  uint64_t WriteTransferCount;
  uint64_t OtherTransferCount;
};
#endif

}  // namespace base

#endif  // BASE_PROCESS_PROCESS_METRICS_INTERNAL_H_
