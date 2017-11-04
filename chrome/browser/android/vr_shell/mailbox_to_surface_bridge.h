// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_VR_SHELL_MAILBOX_TO_SURFACE_BRIDGE_H_
#define CHROME_BROWSER_ANDROID_VR_SHELL_MAILBOX_TO_SURFACE_BRIDGE_H_

#include "base/memory/weak_ptr.h"
#include "ui/gfx/buffer_format_util.h"

namespace gl {
class ScopedJavaSurface;
class SurfaceTexture;
}

namespace gfx {
struct GpuFenceHandle;
struct GpuMemoryBufferHandle;
}

namespace gpu {
class ContextSupport;
struct Mailbox;
struct MailboxHolder;
struct SyncToken;
namespace gles2 {
class GLES2Interface;
}
}

namespace viz {
class ContextProvider;
}

namespace vr_shell {

class MailboxToSurfaceBridge {
 public:
  MailboxToSurfaceBridge(std::unique_ptr<base::OnceClosure> on_initialized);
  ~MailboxToSurfaceBridge();

  void CreateSurface(gl::SurfaceTexture*);

  void ResizeSurface(int width, int height);

  // Returns true if swapped successfully. This can fail if the GL
  // context isn't ready for use yet, in that case the caller
  // won't get a new frame on the SurfaceTexture.
  bool CopyMailboxToSurfaceAndSwap(const gpu::MailboxHolder& mailbox);

  void FetchGpuFence(const gpu::SyncToken& sync_token,
                     const base::Callback<void(const gfx::GpuFenceHandle&)>& callback);
  void PlaceGpuFence(//gpu::SyncToken&,
                     const gfx::GpuFenceHandle&);

  void GenerateMailbox(gpu::Mailbox& out_mailbox);
  void ProduceSharedBuffer(const gpu::Mailbox&,
                           gpu::SyncToken& sync_token,
                           const gfx::GpuMemoryBufferHandle&,
                           const gfx::Size& size,
                           gfx::BufferFormat format,
                           gfx::BufferUsage usage);

 private:
  void OnContextAvailable(std::unique_ptr<gl::ScopedJavaSurface> surface,
                          scoped_refptr<viz::ContextProvider>);
  void InitializeRenderer();
  void DestroyContext();
  void DrawQuad(unsigned int textureHandle);

  scoped_refptr<viz::ContextProvider> context_provider_;
  gpu::gles2::GLES2Interface* gl_ = nullptr;
  gpu::ContextSupport* context_support_ = nullptr;
  int surface_handle_ = 0;
  std::unique_ptr<base::OnceClosure> on_initialized_;

  // Must be last.
  base::WeakPtrFactory<MailboxToSurfaceBridge> weak_ptr_factory_;
};

}  // namespace vr_shell

#endif  // CHROME_BROWSER_ANDROID_VR_SHELL_MAILBOX_TO_SURFACE_BRIDGE_H_
