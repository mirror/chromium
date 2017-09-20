// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/task_manager/providers/task_provider_observer.h"

namespace task_manager {

void TaskProviderObserver::TaskReplaced(Task* old_task, Task* new_task) {
  TaskAdded(new_task);
  TaskRemoved(old_task);
}

void TaskProviderObserver::TaskUnresponsive(Task* task) {}

}  // namespace task_manager
