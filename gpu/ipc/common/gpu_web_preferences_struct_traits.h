// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_COMMON_GPU_WEB_PREFERENCES_STRUCT_TRAITS_H_
#define GPU_IPC_COMMON_GPU_WEB_PREFERENCES_STRUCT_TRAITS_H_

#include "gpu/ipc/common/gpu_web_preferences.mojom.h"

namespace mojo {

template <>
struct StructTraits<gpu::mojom::GpuWebPreferencesDataView,
                    gpu::GpuWebPreferences> {
  static bool Read(gpu::mojom::GpuWebPreferencesDataView data,
		   gpu::GpuWebPreferences* out);

  static bool disable_2d_canvas_copy_on_write(
      const gpu::GpuWebPreferences& gpu_web_preferences) {
    return gpu_web_preferences.disable_2d_canvas_copy_on_write;
  }
};

}  // namespace mojo

#endif  // GPU_IPC_COMMON_GPU_WEB_PREFERENCES_STRUCT_TRAITS_H_
