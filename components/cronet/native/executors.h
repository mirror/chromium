// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_NATIVE_EXECUTORS_H_
#define COMPONENTS_CRONET_NATIVE_EXECUTORS_H_

#include "components/cronet/native/generated/cronet.idl_c.h"

#ifdef __cplusplus
extern "C" {
#endif

// Create Executor that executes runnables directly on caller thread.
Cronet_ExecutorPtr DirectExecutor_Create();

// Create Executor that executes runnables using base::SequencedTaskRunner().
Cronet_ExecutorPtr TaskRunnerExecutor_Create();

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_CRONET_NATIVE_EXECUTORS_H_
