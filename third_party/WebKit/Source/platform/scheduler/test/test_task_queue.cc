// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scheduler/test/test_task_queue.h"

namespace blink {
namespace scheduler {

TestTaskQueue::TestTaskQueue(internal::TaskQueueImpl* impl) : TaskQueue(impl) {}

TestTaskQueue::~TestTaskQueue() {}

}  // namespace scheduler
}  // namespace blink
