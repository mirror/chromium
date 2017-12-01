// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_NATIVE_CRONET_C_H_
#define COMPONENTS_CRONET_NATIVE_CRONET_C_H_

#if defined(WIN32)
#define CRONET_EXPORT
#else
#define CRONET_EXPORT __attribute__((visibility("default")))
#endif

// Cronet public C API is generated from cronet.idl and
#include "cronet.idl_c.h"

#ifdef __cplusplus
extern "C" {
#endif

// Create Executor that executes runnables directly on caller thread.
CRONET_EXPORT Cronet_ExecutorPtr Cronet_Executor_CreateDirect();

// Create Executor that executes runnables using base::SequencedTaskRunner().
CRONET_EXPORT Cronet_ExecutorPtr Cronet_Executor_CreateTaskRunner();

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_CRONET_NATIVE_CRONET_C_H_
