// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/test/test_task_queue.h"

#include "platform/scheduler/base/task_queue_impl.h"

namespace blink {
namespace scheduler {

TestTaskQueue::TestTaskQueue(
    std::unique_ptr<internal::TaskQueueImpl> impl,
    scoped_refptr<base::SingleThreadTaskRunner> shutdown_task_runner)
    : TaskQueue(std::move(impl), shutdown_task_runner),
      automatically_unregister_(!shutdown_task_runner),
      weak_factory_(this) {}

TestTaskQueue::~TestTaskQueue() {
  if (automatically_unregister_) {
    // Automatically unregisters task queue upon deletion, given the fact
    // that TestTaskQueue lives on the main thread.
    UnregisterTaskQueue();
  }
}

base::WeakPtr<TestTaskQueue> TestTaskQueue::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace scheduler
}  // namespace blink
