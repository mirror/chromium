// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_NATIVE_INCLUDE_CRONET_C_H_
#define COMPONENTS_CRONET_NATIVE_INCLUDE_CRONET_C_H_

#include "cronet_export.h"

// Cronet public C API is generated from cronet.idl and
#include "cronet.idl_c.h"

#ifdef __cplusplus
extern "C" {
#endif

// Create Executor that executes runnables directly on caller thread.
CRONET_EXPORT Cronet_ExecutorPtr Cronet_Executor_CreateDirect();

// Create Executor that executes runnables using base::SequencedTaskRunner().
CRONET_EXPORT Cronet_ExecutorPtr Cronet_Executor_CreateTaskRunner();

///////////////////////////////////
// Test support

// Initialize Cronet environment for testing.
CRONET_EXPORT void Cronet_InitializeForTesting();

// Shutdown Cronet environment for testing.
CRONET_EXPORT void Cronet_ShutdownForTesting();

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_CRONET_NATIVE_INCLUDE_CRONET_C_H_
