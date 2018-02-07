// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_GCM_DRIVER_GCM_DELAYED_TASK_CONTROLLER_H_
#define COMPONENTS_GCM_DRIVER_GCM_DELAYED_TASK_CONTROLLER_H_

#include <vector>

#include "base/callback.h"
#include "base/macros.h"

namespace gcm {

// Helper class to save tasks to run until we're ready to execute them.
class GCMDelayedTaskController {
 public:
  GCMDelayedTaskController();
  ~GCMDelayedTaskController();

  // Adds |task| to be invoked once we're ready. The |task| will be posted to
  // the task runner for the current thread if we're ready already.
  void AddTask(base::OnceClosure task);

  // Sets ready status, which will release all of the pending tasks. This method
  // must only be called once.
  void SetReady();

 private:
  friend class GCMDelayedTaskControllerTest;

  // Returns whether we are ready to perform tasks.
  bool CanRunTaskWithoutDelayForTesting() const { return ready_; }

  // Flag that indicates that controlled component is ready.
  bool ready_ = false;

  std::vector<base::OnceClosure> delayed_tasks_;

  DISALLOW_COPY_AND_ASSIGN(GCMDelayedTaskController);
};

}  // namespace gcm

#endif  // COMPONENTS_GCM_DRIVER_GCM_DELAYED_TASK_CONTROLLER_H_
