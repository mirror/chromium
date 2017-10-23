// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_TASK_MANAGER_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_TASK_MANAGER_H_

#include <map>
#include <memory>

#include "base/macros.h"
#include "base/optional.h"
#include "components/download/internal/stats.h"
#include "components/download/public/download_service.h"
#include "components/download/public/task_scheduler.h"

namespace download {

// Handles storing and maintaining the calls made to the TaskScheduler.  This
// class is responsible for abstracting away the expectations of the
// TaskScheduler from the calling code.
class TaskManager {
 public:
  // TaskParams are used to create calls to TaskScheduler::ScheduleTask().
  struct TaskParams {
    TaskParams();
    ~TaskParams() = default;
    bool operator==(const TaskParams& other) const;

    bool requires_unmetered_network;
    bool requires_battery_charging;
    long window_start_seconds;
    long window_end_seconds;
  };

  TaskManager(std::unique_ptr<TaskScheduler> scheduler);
  virtual ~TaskManager();

  // To be called when the system starts a specific task through the
  // DownloadService API.  It is expected that a task of a specific type cannot
  // be started more than once without a call to FinishTask() first.
  void OnSystemStartedTask(DownloadTaskType type,
                           const TaskFinishedCallback& callback);

  // Used to schedule a new type of task.  This will not actually schedule the
  // task until the task of the existing type is finished.
  void ScheduleTask(DownloadTaskType type, const TaskParams& params);

  // Used to cancel a scheduled task.  This will not actually cancel the task if
  // the task is already running, but it will prevent re-scheduling in this
  // case.  If the task is not running it will cancel any scheduled task.
  void UnscheduleTask(DownloadTaskType type);

  // Called when a task is complete.  This method will respond to |callback|
  // passed into OnSystemStartedTask() if |status| is not CANCELLED_ON_STOP.
  // It will also attempt to reschedule the task if it has no parameters to
  // change from previous calls to |ScheduleTask| and if |needs_more_work| is
  // |true|.
  void FinishTask(DownloadTaskType type,
                  bool needs_more_work,
                  stats::ScheduledTaskStatus status);

 private:
  bool IsRunning(DownloadTaskType type);

  std::unique_ptr<TaskScheduler> scheduler_;

  std::map<DownloadTaskType, base::Optional<TaskParams>> scheduled_params_;
  std::map<DownloadTaskType, TaskFinishedCallback> running_tasks_;

  DISALLOW_COPY_AND_ASSIGN(TaskManager);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_TASK_MANAGER_H_
