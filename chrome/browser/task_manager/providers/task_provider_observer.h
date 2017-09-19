// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TASK_MANAGER_PROVIDERS_TASK_PROVIDER_OBSERVER_H_
#define CHROME_BROWSER_TASK_MANAGER_PROVIDERS_TASK_PROVIDER_OBSERVER_H_

#include "base/macros.h"
#include "chrome/browser/task_manager/providers/task.h"

namespace task_manager {

// Defines an interface for observing tasks addition / removal.
class TaskProviderObserver {
 public:
  TaskProviderObserver() {}
  virtual ~TaskProviderObserver() {}

  // This notifies of the event that a new |task| has been created.
  virtual void TaskAdded(Task* task) = 0;

  // This notifies of the event that a |task| is about to be removed. The task
  // is still valid during this call, after that it may never be used again by
  // the observer and references to it must not be kept.
  virtual void TaskRemoved(Task* task) = 0;

  // Notifies of a simultaneous Add & Remove event. Both |old_task| and
  // |new_task| are valid during this call. After the observer returns from this
  // call, |old_task| may never be used again by the observer; references to it
  // must not be kept.
  //
  // Overriding this method is optional. If an observer implementation chooses
  // not to override this method, they'll just see TaskAdded(new_task), followed
  // by TaskReplaced(old_task).
  virtual void TaskReplaced(Task* old_task, Task* new_task);

  // This notifies of the event that |task| has become unresponsive. This event
  // is only for tasks representing renderer processes.
  virtual void TaskUnresponsive(Task* task);

 private:
  DISALLOW_COPY_AND_ASSIGN(TaskProviderObserver);
};

}  // namespace task_manager

#endif  // CHROME_BROWSER_TASK_MANAGER_PROVIDERS_TASK_PROVIDER_OBSERVER_H_
