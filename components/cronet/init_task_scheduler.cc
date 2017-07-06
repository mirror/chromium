// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/init_task_scheduler.h"

#include "base/task_scheduler/task_scheduler.h"

namespace cronet {

void InitTaskScheduler() {
  base::TaskScheduler::CreateAndStartWithDefaultParams("Cronet");
}

void ShutdownTaskScheduler() {
  base::TaskScheduler* ts = base::TaskScheduler::GetInstance();
  if (ts)
    ts->Shutdown();
}

}  // namespace cronet
