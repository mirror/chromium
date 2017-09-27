// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_background_task.h"

#include "components/offline_pages/core/prefetch/prefetch_background_task_handler.h"
#include "components/offline_pages/core/prefetch/prefetch_dispatcher.h"
#include "components/offline_pages/core/prefetch/prefetch_service.h"

namespace offline_pages {

PrefetchBackgroundTask::PrefetchBackgroundTask(PrefetchService* service)
    : service_(service) {}

PrefetchBackgroundTask::~PrefetchBackgroundTask() {
  PrefetchBackgroundTaskHandler* handler =
      service_->GetPrefetchBackgroundTaskHandler();
  if (reschedule_type_ ==
      PrefetchBackgroundTaskRescheduleType::RESCHEDULE_WITH_BACKOFF) {
    handler->Backoff();
  } else {
    handler->ResetBackoff();
  }

  if (NeedsReschedule())
    handler->EnsureTaskScheduled();
}

void PrefetchBackgroundTask::SetReschedule(
    PrefetchBackgroundTaskRescheduleType type) {
  switch (type) {
    // |SUSPEND| takes highest precendence.
    case PrefetchBackgroundTaskRescheduleType::SUSPEND:
      reschedule_type_ = PrefetchBackgroundTaskRescheduleType::SUSPEND;
      break;
    // |RESCHEDULE_WITH_BACKOFF| takes 2nd highest precendence and thus it can't
    // overwrite |SUSPEND|.
    case PrefetchBackgroundTaskRescheduleType::RESCHEDULE_WITH_BACKOFF:
      if (reschedule_type_ != PrefetchBackgroundTaskRescheduleType::SUSPEND) {
        reschedule_type_ =
            PrefetchBackgroundTaskRescheduleType::RESCHEDULE_WITH_BACKOFF;
      }
      break;
    // |RESCHEDULE_WITHOUT_BACKOFF| takes 3rd highest precendence and thus it
    // only overwrites the lowest precendence |NO_RESCHEDULE|.
    case PrefetchBackgroundTaskRescheduleType::RESCHEDULE_WITHOUT_BACKOFF:
      if (reschedule_type_ ==
          PrefetchBackgroundTaskRescheduleType::NO_RESCHEDULE) {
        reschedule_type_ =
            PrefetchBackgroundTaskRescheduleType::RESCHEDULE_WITHOUT_BACKOFF;
      }
      break;
    case PrefetchBackgroundTaskRescheduleType::NO_RESCHEDULE:
      break;
  }
}

void PrefetchBackgroundTask::Stop() {
  task_killed_by_system_ = true;
  service_->GetPrefetchDispatcher()->StopBackgroundTask();
}

void PrefetchBackgroundTask::FinishForTesting() {
  service_->GetPrefetchDispatcher()->RequestFinishBackgroundTaskForTest();
}

int PrefetchBackgroundTask::GetAdditionalSeconds() const {
  // The task should be rescheduled after 1 day if the suspension is requested.
  // This will happen even when the task is then killed due to the system.
  if (reschedule_type_ == PrefetchBackgroundTaskRescheduleType::SUSPEND)
    return static_cast<int>(base::TimeDelta::FromDays(1).InSeconds());

  // If the task is killed due to the system, it should be rescheduled without
  // backoff even when it is in effect because we want to rerun the task asap.
  if (task_killed_by_system_)
    return 0;

  return service_->GetPrefetchBackgroundTaskHandler()
      ->GetAdditionalBackoffSeconds();
}

bool PrefetchBackgroundTask::NeedsReschedule() const {
  return reschedule_type() !=
             PrefetchBackgroundTaskRescheduleType::NO_RESCHEDULE ||
         task_killed_by_system_;
}

}  // namespace offline_pages
