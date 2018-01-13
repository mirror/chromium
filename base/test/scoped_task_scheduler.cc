// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/scoped_task_scheduler.h"

#include "base/task_scheduler/task_scheduler.h"

namespace base {
namespace test {

ScopedTaskScheduler::ScopedTaskScheduler(StringPiece name) {
  TaskScheduler::CreateAndStartWithDefaultParams(name);
}

ScopedTaskScheduler::~ScopedTaskScheduler() {
  base::TaskScheduler::GetInstance()->FlushForTesting();
  base::TaskScheduler::GetInstance()->Shutdown();
  base::TaskScheduler::GetInstance()->JoinForTesting();
  base::TaskScheduler::SetInstance(nullptr);
}

}  // namespace test
}  // namespace base
