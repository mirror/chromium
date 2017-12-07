// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/include/cronet_c.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/task_scheduler/post_task.h"
#include "base/task_scheduler/task_traits.h"
#include "components/cronet/native/generated/cronet.idl_impl_interface.h"

namespace {

// Implementation of DirectExecutor methods.
void DirectExecutor_Execute(Cronet_ExecutorPtr self,
                            Cronet_RunnablePtr command) {
  CHECK(self);
  Cronet_Runnable_Run(command);
  Cronet_Runnable_Destroy(command);
}

// Implelementation of TaskRunnerExecutor.
class TaskRunnerExecutor : public Cronet_Executor {
 public:
  TaskRunnerExecutor()
      : task_runner_(CreateSequencedTaskRunnerWithTraits(base::TaskTraits())) {}
  ~TaskRunnerExecutor() override = default;

  void SetContext(Cronet_ExecutorContext context) override {
    context_ = context;
  }

  Cronet_ExecutorContext GetContext() override { return context_; }

  void Execute(Cronet_RunnablePtr command) override;

 private:
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  Cronet_ExecutorContext context_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TaskRunnerExecutor);
};

void TaskRunnerExecutor::Execute(Cronet_RunnablePtr command) {
  task_runner_->PostTask(FROM_HERE, base::BindOnce(
                                        [](Cronet_RunnablePtr command) {
                                          DVLOG(1) << "Running posted task";
                                          Cronet_Runnable_Run(command);
                                          DVLOG(1) << "Destroying command";
                                          Cronet_Runnable_Destroy(command);
                                        },
                                        command));
}

}  // namespace

Cronet_ExecutorPtr Cronet_Executor_CreateDirect() {
  return Cronet_Executor_CreateStub(DirectExecutor_Execute);
}

Cronet_ExecutorPtr Cronet_Executor_CreateTaskRunner() {
  return new TaskRunnerExecutor();
}
