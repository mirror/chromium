// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_NATIVE_INCLUDE_CRONET_C_H_
#define COMPONENTS_CRONET_NATIVE_INCLUDE_CRONET_C_H_

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

// Create Buffer that allocates |data| of |size| using Cronet memory allocator.
CRONET_EXPORT Cronet_BufferPtr Cronet_Buffer_Allocate(uint64_t size);

// Create Buffer that wraps app-allocated |data| of |size|. The
// |on_destroy_func| is called when buffer is destroyed, so |data| can be freed.
CRONET_EXPORT Cronet_BufferPtr Cronet_Buffer_CreateWithDataAndOnDestroyFunc(
    void* data,
    uint64_t size,
    Cronet_BufferCallback_OnDestroyFunc on_destroy_func);

#ifdef __cplusplus
}
#endif

#endif  // COMPONENTS_CRONET_NATIVE_INCLUDE_CRONET_C_H_
