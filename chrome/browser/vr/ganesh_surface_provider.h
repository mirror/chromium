// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_GANESH_SURFACE_PROVIDER_H_
#define CHROME_BROWSER_VR_GANESH_SURFACE_PROVIDER_H_

#include "chrome/browser/vr/skia_surface_provider.h"

class GrContext;
struct GrGLInterface;

namespace vr {

class GaneshSurfaceProvider : public SkiaSurfaceProvider {
 public:
  GaneshSurfaceProvider();
  ~GaneshSurfaceProvider() override;

  sk_sp<SkSurface> MakeSurface(const gfx::Size& size) override;
  void FlushSurface(SkSurface* surface, GLuint* out_texure_id) override;

 private:
  sk_sp<GrContext> gr_context_;
  sk_sp<const GrGLInterface> gr_interface_;
  GLint main_fbo_ = 0;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_GANESH_SURFACE_PROVIDER_H_
