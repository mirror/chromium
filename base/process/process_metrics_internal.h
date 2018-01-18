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
#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

namespace base {

#if defined(OS_WIN)
struct IoCounters : public IO_COUNTERS {};
#endif

}  // namespace base

#endif  // BASE_PROCESS_PROCESS_METRICS_INTERNAL_H_
