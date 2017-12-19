// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cronet/native/test_util.h"

#include "base/bind.h"
#include "base/task_scheduler/post_task.h"

namespace {
// Implementation of PostTaskExecutor methods.
void TestExecutor_Execute(Cronet_ExecutorPtr self, Cronet_RunnablePtr command) {
  CHECK(self);
  DVLOG(1) << "Post Task";
  base::PostTask(FROM_HERE, base::BindOnce(
                                [](Cronet_RunnablePtr command) {
                                  DVLOG(1) << "Running posted task";
                                  Cronet_Runnable_Run(command);
                                  DVLOG(1) << "Destroying command";
                                  Cronet_Runnable_Destroy(command);
                                },
                                command));
}

}  // namespace

namespace cronet {

// static
Cronet_ExecutorPtr TestUtil::CreateTestExecutor() {
  return Cronet_Executor_CreateStub(TestExecutor_Execute);
}

}  // namespace cronet
