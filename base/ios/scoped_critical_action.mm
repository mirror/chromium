// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/ios/scoped_critical_action.h"

#import <UIKit/UIKit.h>

#include "base/lazy_instance.h"
#include "base/synchronization/lock.h"

namespace base {
namespace ios {
namespace {

class BackgroundTaskTracker {
 public:
  int BeginBackgroundTask() {
    UIApplication* application = [UIApplication sharedApplication];
    if (!application) {
      return -1;
    }

    AutoLock lock_scope(lock_);

    int task_id = next_task_id_++;

    ++running_task_count_;
    if (running_task_count_ == 1) {
      background_task_id_ =
          [application beginBackgroundTaskWithExpirationHandler:^{
            OnBackgroundTaskExpired();
          }];
    }

    return task_id;
  }

  void EndBackgroundTask(int task_id) {
    UIApplication* application = [UIApplication sharedApplication];
    if (!application) {
      return;
    }

    UIBackgroundTaskIdentifier end_task = UIBackgroundTaskInvalid;

    {
      AutoLock lock_scope(lock_);

      if (task_id < max_expired_task_id_) {
        // Ignore expired tasks.
        return;
      }

      --running_task_count_;
      if (running_task_count_ == 0) {
        std::swap(background_task_id_, end_task);
      }
    }

    if (end_task != UIBackgroundTaskInvalid) {
      [application endBackgroundTask:end_task];
    }
  }

  void OnBackgroundTaskExpired() {
    UIBackgroundTaskIdentifier end_task = UIBackgroundTaskInvalid;

    {
      AutoLock lock_scope(lock_);

      // Mark all previous tasks as expired.
      max_expired_task_id_ = next_task_id_;
      // No running tasks at this moment.
      running_task_count_ = 0;

      std::swap(background_task_id_, end_task);
    }

    if (end_task != UIBackgroundTaskInvalid) {
      [[UIApplication sharedApplication] endBackgroundTask:end_task];
    }
  }

 private:
  Lock lock_;
  // Number of running tasks. First running task begins system background task
  // |background_task_id_| with |beginBackgroundTaskWithExpirationHandler|,
  // last running task ends system background task |background_task_id_|
  // with |endBackgroundTask|.
  int running_task_count_ = 0;
  // Generator for task id.
  int next_task_id_ = 0;
  // Tasks with task_id < |max_expired_task_id_| are expired and should be
  // ignored in |EndBackgroundTask|.
  int max_expired_task_id_ = 0;
  // Task id for system background task.
  UIBackgroundTaskIdentifier background_task_id_ = UIBackgroundTaskInvalid;
};

LazyInstance<BackgroundTaskTracker>::Leaky g_background_task_tracker =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

ScopedCriticalAction::ScopedCriticalAction()
    : task_id_(g_background_task_tracker.Get().BeginBackgroundTask()) {}

ScopedCriticalAction::~ScopedCriticalAction() {
  g_background_task_tracker.Get().EndBackgroundTask(task_id_);
}

}  // namespace ios
}  // namespace base
