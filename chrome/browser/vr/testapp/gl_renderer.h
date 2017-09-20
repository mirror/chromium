// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TESTAPP_GL_RENDERER_H_
#define CHROME_BROWSER_VR_TESTAPP_GL_RENDERER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/vr/testapp/renderer_base.h"
#include "ui/gfx/swap_result.h"

namespace gl {
class GLContext;
class GLSurface;
}  // namespace gl

namespace vr {
class VrGlLinux;
}

namespace ui {

class GlRenderer : public RendererBase {
 public:
  GlRenderer(gfx::AcceleratedWidget widget,
             const scoped_refptr<gl::GLSurface>& surface,
             const gfx::Size& size);
  ~GlRenderer() override;

  // Renderer:
  bool Initialize() override;

 protected:
  virtual void RenderFrame();
  virtual void PostRenderFrameTask(gfx::SwapResult result);

  scoped_refptr<gl::GLSurface> surface_;
  scoped_refptr<gl::GLContext> context_;

 private:
  std::unique_ptr<vr::VrGlLinux> vr_gl_;

  base::WeakPtrFactory<GlRenderer> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(GlRenderer);
};

}  // namespace ui

#endif  // CHROME_BROWSER_VR_TESTAPP_GL_RENDERER_H_
