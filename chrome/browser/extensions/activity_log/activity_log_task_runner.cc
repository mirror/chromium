// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/activity_log/activity_log_task_runner.h"

#include "base/single_thread_task_runner.h"
#include "base/task_scheduler/lazy_task_runner.h"
#include "base/task_scheduler/single_thread_task_runner_thread_mode.h"

#include "debug/stack_trace.h"

namespace extensions {

namespace {

base::SingleThreadTaskRunner* g_task_runner_for_testing = nullptr;

base::LazySingleThreadTaskRunner g_task_runner =
    LAZY_SINGLE_THREAD_TASK_RUNNER_INITIALIZER(
        base::TaskTraits({base::MayBlock(), base::TaskPriority::BACKGROUND}),
        base::SingleThreadTaskRunnerThreadMode::SHARED);

base::debug::StackTrace* g_creation_stack = nullptr;

}  // namespace

const scoped_refptr<base::SingleThreadTaskRunner> GetActivityLogTaskRunner() {
  if (g_task_runner_for_testing)
    return g_task_runner_for_testing;

  if (!g_creation_stack)
    g_creation_stack = new base::debug::StackTrace;

  return g_task_runner.Get();
}

void SetActivityLogTaskRunnerForTesting(
    base::SingleThreadTaskRunner* task_runner) {
  DCHECK(!g_creation_stack) << task_runner << " and stack:\n"
                            << g_creation_stack->ToString();

  g_task_runner_for_testing = task_runner;
}

}  // namespace extensions
