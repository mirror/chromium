// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_APP_PROFILER_SCOPED_STACK_SAMPLING_PROFILER_H_
#define CHROME_APP_PROFILER_SCOPED_STACK_SAMPLING_PROFILER_H_

#include <memory>

#include "base/macros.h"
#include "base/profiler/stack_sampling_profiler.h"

// A wrapper class that begins profiling stack samples upon construction, and
// ensures correct shutdown behavior on destruction. Samples are collected for
// the active thread of the current process, and only if profiling is enabled
// for the thread. This data is used to understand startup performance behavior,
// and the object should therefore be created as early during initialization as
// possible.
class ScopedStackSamplingProfiler {
 public:
  ScopedStackSamplingProfiler();
  ~ScopedStackSamplingProfiler();

 private:
  // A profiler that periodically samples stack traces. Used to understand
  // thread and process startup behavior.
  std::unique_ptr<base::StackSamplingProfiler> sampling_profiler_;

  DISALLOW_COPY_AND_ASSIGN(ScopedStackSamplingProfiler);
};

#endif  //  CHROME_APP_PROFILER_SCOPED_STACK_SAMPLING_PROFILER_H_
