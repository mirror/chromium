// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_VR_SURFACE_PROVIDER_H_
#define CHROME_BROWSER_VR_VR_SURFACE_PROVIDER_H_

#include "third_party/skia/include/core/SkRefCnt.h"

class SkSurface;

namespace vr {

class VrSurfaceProvider {
 public:
  virtual ~VrSurfaceProvider() = default;

  virtual sk_sp<SkSurface> MakeSurface(int width, int height) = 0;
};

}  // namespace vr

#endif  // CHROME_BROWSER_VR_VR_SURFACE_PROVIDER_H_