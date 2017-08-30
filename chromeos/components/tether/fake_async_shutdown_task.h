// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_TETHER_FAKE_ASYNC_SHUTDOWN_TASK_H_
#define CHROMEOS_COMPONENTS_TETHER_FAKE_ASYNC_SHUTDOWN_TASK_H_

#include "chromeos/components/tether/async_shutdown_task.h"

namespace chromeos {

namespace tether {

// Test double for AsyncShutdownTask.
class FakeAsyncShutdownTask : public AsyncShutdownTask {
 public:
  FakeAsyncShutdownTask();
  ~FakeAsyncShutdownTask() override;

  void NotifyAsyncShutdownComplete();

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeAsyncShutdownTask);
};

}  // namespace tether

}  // namespace chromeos

#endif  // CHROMEOS_COMPONENTS_TETHER_FAKE_ASYNC_SHUTDOWN_TASK_H_
