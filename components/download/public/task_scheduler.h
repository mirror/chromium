// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_PUBLIC_TASK_SCHEDULER_H_
#define COMPONENTS_DOWNLOAD_PUBLIC_TASK_SCHEDULER_H_

#include "components/download/public/download_task_types.h"

namespace download {

class TaskScheduler {
 public:
  // Schedules a task with the operating system.
  virtual void ScheduleTask(DownloadTaskType task_type,
                            bool require_unmetered_network,
                            bool require_charging,
                            long window_start_time_seconds,
                            long window_end_time_seconds) = 0;

  // Cancels a pre-scheduled task.
  virtual void CancelTask(DownloadTaskType task_type) = 0;
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_PUBLIC_TASK_SCHEDULER_H_
