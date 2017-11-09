// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TASK_SCHEDULER_SEQUENCE_MANAGER_GRACEFUL_QUEUE_SHUTDOWN_HELPER_H_
#define BASE_TASK_SCHEDULER_SEQUENCE_MANAGER_GRACEFUL_QUEUE_SHUTDOWN_HELPER_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"

namespace base {

namespace internal {

class TaskQueueImpl;

// Thread-safe helper to shutdown queues from any thread.
class GracefulQueueShutdownHelper
    : public base::RefCountedThreadSafe<GracefulQueueShutdownHelper> {
 public:
  GracefulQueueShutdownHelper();
  ~GracefulQueueShutdownHelper();

  void GracefullyShutdownTaskQueue(
      std::unique_ptr<internal::TaskQueueImpl> queue);

  void OnTaskQueueManagerDeleted();

  std::vector<std::unique_ptr<internal::TaskQueueImpl>> TakeQueues();

 private:
  base::Lock lock_;
  bool task_queue_manager_deleted_;
  std::vector<std::unique_ptr<internal::TaskQueueImpl>> queues_;

  DISALLOW_COPY_AND_ASSIGN(GracefulQueueShutdownHelper);
};

}  // namespace internal

}  // namespace base

#endif  // BASE_TASK_SCHEDULER_SEQUENCE_MANAGER_GRACEFUL_QUEUE_SHUTDOWN_HELPER_H_
