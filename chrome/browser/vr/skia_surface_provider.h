// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_SKIA_SURFACE_PROVIDER_H_
#define CHROME_BROWSER_VR_SKIA_SURFACE_PROVIDER_H_

#include "third_party/skia/include/core/SkRefCnt.h"
#include "ui/gl/gl_bindings.h"

class SkSurface;

namespace gfx {
class Size;
}  // namespace gfx

namespace vr {

class SkiaSurfaceProvider {
 public:
  virtual ~SkiaSurfaceProvider() = default;

  virtual sk_sp<SkSurface> MakeSurface(const gfx::Size& size) = 0;
  virtual void FlushSurface(SkSurface* surface, GLuint* out_texure_id) = 0;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_SKIA_SURFACE_PROVIDER_H_
