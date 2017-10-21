// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/task_manager.h"

#include "base/bind.h"
#include "base/callback_helpers.h"

namespace download {

TaskManager::TaskParams::TaskParams()
    : requires_unmetered_network(true),
      requires_battery_charging(true),
      window_start_seconds(0),
      window_end_seconds(0) {}

bool TaskManager::TaskParams::operator==(const TaskParams& other) const {
  return requires_unmetered_network == other.requires_unmetered_network &&
         requires_battery_charging == other.requires_battery_charging &&
         window_start_seconds == other.window_start_seconds &&
         window_end_seconds == other.window_end_seconds;
}

TaskManager::TaskManager(std::unique_ptr<TaskScheduler> scheduler)
    : scheduler_(std::move(scheduler)) {}

TaskManager::~TaskManager() = default;

void TaskManager::OnSystemStartedTask(DownloadTaskType type,
                                      const TaskFinishedCallback& callback) {
  DCHECK(!IsRunning(type));
  running_tasks_[type] = callback;
}

void TaskManager::ScheduleTask(DownloadTaskType type,
                               const TaskParams& params) {
  auto it = scheduled_params_.find(type);
  if (it != scheduled_params_.end() && it->second == params)
    return;
  scheduled_params_[type] = params;

  if (IsRunning(type))
    return;

  scheduler_->ScheduleTask(
      type, params.requires_unmetered_network, params.requires_battery_charging,
      params.window_start_seconds, params.window_end_seconds);
}

void TaskManager::UnscheduleTask(DownloadTaskType type) {
  auto it = scheduled_params_.find(type);
  if (it == scheduled_params_.end())
    return;
  scheduled_params_.erase(it);

  if (IsRunning(type))
    return;

  scheduler_->CancelTask(type);
}

void TaskManager::FinishTask(DownloadTaskType type,
                             bool needs_more_work,
                             stats::ScheduledTaskStatus status) {
  if (!IsRunning(type))
    return;

  stats::LogScheduledTaskStatus(type, status);

  auto params = scheduled_params_.find(type);
  bool has_params = params != scheduled_params_.end();

  if (status != stats::ScheduledTaskStatus::CANCELLED_ON_STOP) {
    // Reschedule if (1) we have more work and (2) we don't have new params to
    // schedule.
    bool reschedule = needs_more_work && !has_params;
    running_tasks_[type].Run(reschedule);
  }
  running_tasks_.erase(type);

  if (has_params) {
    scheduler_->ScheduleTask(type, params->second.requires_unmetered_network,
                             params->second.requires_battery_charging,
                             params->second.window_start_seconds,
                             params->second.window_end_seconds);
  } else {
    // This is a precaution.  We should not have to do this.
    scheduler_->CancelTask(type);
  }
}

bool TaskManager::IsRunning(DownloadTaskType type) {
  return running_tasks_.find(type) != running_tasks_.end();
}

}  // namespace download
