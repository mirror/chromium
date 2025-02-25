// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_CHILD_WEB_TASK_RUNNER_IMPL_H_
#define THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_CHILD_WEB_TASK_RUNNER_IMPL_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/optional.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"
#include "platform/PlatformExport.h"
#include "public/platform/TaskType.h"

namespace blink {
namespace scheduler {
class TaskQueue;

// TODO(yutak): WebTaskRunner is no more; rename the class.
class PLATFORM_EXPORT WebTaskRunnerImpl : public base::SingleThreadTaskRunner {
 public:
  static scoped_refptr<WebTaskRunnerImpl> Create(
      scoped_refptr<TaskQueue> task_queue,
      base::Optional<TaskType> task_type);

  // base::SingleThreadTaskRunner implementation:
  bool RunsTasksInCurrentSequence() const override;

  TaskQueue* GetTaskQueue() const { return task_queue_.get(); }

 protected:
  bool PostDelayedTask(const base::Location&,
                       base::OnceClosure,
                       base::TimeDelta) override;
  bool PostNonNestableDelayedTask(const base::Location&,
                                  base::OnceClosure,
                                  base::TimeDelta) override;

 private:
  WebTaskRunnerImpl(scoped_refptr<TaskQueue> task_queue,
                    base::Optional<TaskType> task_type);
  ~WebTaskRunnerImpl() override;

  scoped_refptr<TaskQueue> task_queue_;
  base::Optional<TaskType> task_type_;

  DISALLOW_COPY_AND_ASSIGN(WebTaskRunnerImpl);
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_WEBKIT_SOURCE_PLATFORM_SCHEDULER_CHILD_WEB_TASK_RUNNER_IMPL_H_
