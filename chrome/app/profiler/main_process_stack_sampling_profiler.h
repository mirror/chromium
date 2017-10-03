// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_APP_PROFILER_MAIN_PROCESS_STACK_SAMPLING_PROFILER_H_
#define CHROME_APP_PROFILER_MAIN_PROCESS_STACK_SAMPLING_PROFILER_H_

// Begins profiling stack samples for the main process, if enabled. Used to
// understand startup performance behavior. Should be called as early during
// initialization as possible.
void StartMainProcessStackSamplingProfilerIfEnabled();

#endif  // CHROME_APP_PROFILER_MAIN_PROCESS_STACK_SAMPLING_PROFILER_H_
