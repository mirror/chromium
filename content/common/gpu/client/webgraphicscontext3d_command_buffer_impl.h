// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_GPU_CLIENT_WEBGRAPHICSCONTEXT3D_COMMAND_BUFFER_IMPL_H_
#define CONTENT_COMMON_GPU_CLIENT_WEBGRAPHICSCONTEXT3D_COMMAND_BUFFER_IMPL_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "content/common/content_export.h"
#include "content/common/gpu/client/command_buffer_metrics.h"
#include "gpu/command_buffer/common/gles2_cmd_utils.h"
#include "gpu/ipc/client/command_buffer_proxy_impl.h"
#include "gpu/ipc/common/surface_handle.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "ui/gl/gpu_preference.h"
#include "url/gurl.h"

namespace gpu {

class ContextSupport;
class GpuChannelHost;
struct SharedMemoryLimits;
class TransferBuffer;

namespace gles2 {
class GLES2CmdHelper;
class GLES2Implementation;
class GLES2Interface;
class ShareGroup;
}
}

namespace content {

class WebGraphicsContext3DCommandBufferImpl {
 public:
  enum MappedMemoryReclaimLimit {
    kNoLimit = 0,
  };

  // If surface_handle is not kNullSurfaceHandle, this creates a
  // CommandBufferProxy that renders directly to a view. The view and
  // the associated window must not be destroyed until the returned
  // CommandBufferProxy has been destroyed, otherwise the GPU process might
  // attempt to render to an invalid window handle.
  CONTENT_EXPORT WebGraphicsContext3DCommandBufferImpl();
  CONTENT_EXPORT ~WebGraphicsContext3DCommandBufferImpl();

  gpu::CommandBufferProxyImpl* GetCommandBufferProxy() {
    return command_buffer_.get();
  }

  gpu::gles2::GLES2Implementation* GetImplementation() {
    return real_gl_.get();
  }

  CONTENT_EXPORT bool InitializeOnCurrentThread(
      gpu::SurfaceHandle surface_handle,
      const GURL& active_url,
      gpu::GpuChannelHost* host,
      gfx::GpuPreference gpu_preference,
      bool automatic_flushes,
      const gpu::SharedMemoryLimits& memory_limits,
      gpu::CommandBufferProxyImpl* shared_command_buffer,
      scoped_refptr<gpu::gles2::ShareGroup> share_group,
      const gpu::gles2::ContextCreationAttribHelper& attributes,
      command_buffer_metrics::ContextType context_type);

 private:
  bool MaybeInitializeGL(
      gpu::SurfaceHandle surface_handle,
      const GURL& active_url,
      gpu::GpuChannelHost* host,
      gfx::GpuPreference gpu_preference,
      bool automatic_flushes,
      const gpu::SharedMemoryLimits& memory_limits,
      gpu::CommandBufferProxyImpl* shared_command_buffer,
      scoped_refptr<gpu::gles2::ShareGroup> share_group,
      const gpu::gles2::ContextCreationAttribHelper& attributes,
      command_buffer_metrics::ContextType context_type);

  std::unique_ptr<gpu::CommandBufferProxyImpl> command_buffer_;
  std::unique_ptr<gpu::gles2::GLES2CmdHelper> gles2_helper_;
  std::unique_ptr<gpu::TransferBuffer> transfer_buffer_;
  std::unique_ptr<gpu::gles2::GLES2Implementation> real_gl_;
  std::unique_ptr<gpu::gles2::GLES2Interface> trace_gl_;
};

}  // namespace content

#endif  // CONTENT_COMMON_GPU_CLIENT_WEBGRAPHICSCONTEXT3D_COMMAND_BUFFER_IMPL_H_
