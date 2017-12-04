// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_SERVICE_GLES2_COMMAND_BUFFER_STUB_H_
#define GPU_IPC_SERVICE_GLES2_COMMAND_BUFFER_STUB_H_

#include "base/memory/weak_ptr.h"
#include "gpu/ipc/service/command_buffer_stub_common.h"

namespace gpu {

class GPU_EXPORT GLES2CommandBufferStub
    : public CommandBufferStubCommon,
      public base::SupportsWeakPtr<GLES2CommandBufferStub> {
 public:
  GLES2CommandBufferStub(GpuChannel* channel,
                         const GPUCreateCommandBufferConfig& init_params,
                         CommandBufferId command_buffer_id,
                         SequenceId sequence_id,
                         int32_t stream_id,
                         int32_t route_id);

  // This must leave the GL context associated with the newly-created
  // CommandBufferStubCommon current, so the GpuChannel can initialize
  // the gpu::Capabilities.
  gpu::ContextResult Initialize(
      CommandBufferStubCommon* share_group,
      const GPUCreateCommandBufferConfig& init_params,
      std::unique_ptr<base::SharedMemory> shared_state_shm) override;

  DISALLOW_COPY_AND_ASSIGN(GLES2CommandBufferStub);
};

}  // namespace gpu

#endif  // GPU_IPC_SERVICE_GLES2_COMMAND_BUFFER_STUB_H_
