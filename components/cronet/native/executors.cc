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

}  // namespace

Cronet_ExecutorPtr Cronet_Executor_CreateDirect() {
  return Cronet_Executor_CreateStub(DirectExecutor_Execute);
}
