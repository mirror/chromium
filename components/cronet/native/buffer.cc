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
void Cronet_Buffer_OnDestroyAllocated(Cronet_BufferCallbackPtr self,
                                      Cronet_BufferPtr buffer) {
  free(Cronet_Buffer_get_data(buffer));
  Cronet_BufferCallback_Destroy(self);
}

}  // namespace

// Create Buffer that allocates |data| of |size| using Cronet memory allocator.
CRONET_EXPORT Cronet_BufferPtr Cronet_Buffer_Allocate(uint64_t size) {
  // Callback that uses app provided |on_destroy_func|.
  Cronet_BufferCallbackPtr buffer_callback =
      Cronet_BufferCallback_CreateStub(Cronet_Buffer_OnDestroyAllocated);
  // Create Cronet buffer with app provided data.
  Cronet_BufferPtr buffer = Cronet_Buffer_Create();
  Cronet_Buffer_set_size(buffer, size);
  Cronet_Buffer_set_limit(buffer, size);
  Cronet_Buffer_set_position(buffer, 0);
  Cronet_Buffer_set_data(buffer, malloc(size));
  // TODO(mef): We need to also destroy |buffer_callback|.
  Cronet_Buffer_set_callback(buffer, buffer_callback);
  return buffer;
}

CRONET_EXPORT Cronet_BufferPtr Cronet_Buffer_CreateWithDataAndOnDestroyFunc(
    void* data,
    uint64_t size,
    Cronet_BufferCallback_OnDestroyFunc on_destroy_func) {
  // Callback that uses app provided |on_destroy_func|.
  Cronet_BufferCallbackPtr buffer_callback =
      Cronet_BufferCallback_CreateStub(on_destroy_func);
  // Create Cronet buffer with app provided data.
  Cronet_BufferPtr buffer = Cronet_Buffer_Create();
  Cronet_Buffer_set_size(buffer, size);
  Cronet_Buffer_set_limit(buffer, size);
  Cronet_Buffer_set_position(buffer, 0);
  Cronet_Buffer_set_data(buffer, data);
  // TODO(mef): We need to also destroy |buffer_callback|.
  Cronet_Buffer_set_callback(buffer, buffer_callback);
  return buffer;
}
