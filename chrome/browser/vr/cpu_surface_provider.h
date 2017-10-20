// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_CPU_SURFACE_PROVIDER_H_
#define CHROME_BROWSER_VR_CPU_SURFACE_PROVIDER_H_

#include "chrome/browser/vr/skia_surface_provider.h"

class GrContext;
struct GrGLInterface;

namespace vr {

class CpuSurfaceProvider : public SkiaSurfaceProvider {
 public:
  CpuSurfaceProvider();
  ~CpuSurfaceProvider() override;

  sk_sp<SkSurface> MakeSurface(const gfx::Size& size) override;
  void FlushSurface(SkSurface* surface, GLuint* out_texure_id) override;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_CPU_SURFACE_PROVIDER_H_
