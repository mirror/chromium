// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/child_process_for_testing.h"

#include "base/task_scheduler/task_scheduler.h"

namespace content {

ChildProcessForTesting::ChildProcessForTesting() = default;

ChildProcessForTesting::~ChildProcessForTesting() {
  if (initialized_task_scheduler()) {
    base::TaskScheduler::GetInstance()->Shutdown();
    base::TaskScheduler::GetInstance()->JoinForTesting();
    base::TaskScheduler::SetInstance(nullptr);
  }
}

}  // namespace content
