// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_CONFIG_GPU_WEB_PREFERENCES_H_
#define GPU_CONFIG_GPU_WEB_PREFERENCES_H_

#include "gpu/gpu_export.h"

namespace gpu {

struct GPU_EXPORT GpuWebPreferences {
  bool disable_2d_canvas_copy_on_write = false;
};

}  // namespace gpu

#endif  // GPU_CONFIG_GPU_WEB_PREFERENCES_H_
