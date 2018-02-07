// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/gcm_driver/gcm_delayed_task_controller.h"

#include <stddef.h>

#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"

namespace gcm {

GCMDelayedTaskController::GCMDelayedTaskController() = default;

GCMDelayedTaskController::~GCMDelayedTaskController() = default;

void GCMDelayedTaskController::AddTask(base::OnceClosure task) {
  if (ready_)
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, std::move(task));
  else
    delayed_tasks_.push_back(std::move(task));
}

void GCMDelayedTaskController::SetReady() {
  DCHECK(!ready_);
  ready_ = true;

  for (auto& callback : delayed_tasks_)
    std::move(callback).Run();

  delayed_tasks_.clear();
}

}  // namespace gcm
