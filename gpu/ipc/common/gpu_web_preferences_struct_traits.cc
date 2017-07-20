// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/common/gpu_web_preferences_struct_traits.h"

namespace mojo {

// static
bool StructTraits<gpu::mojom::GpuWebPreferencesDataView,
		  gpu::GpuWebPreferences>::Read(
    gpu::mojom::GpuWebPreferencesDataView data,
    gpu::GpuWebPreferences* out) {
  out->disable_2d_canvas_copy_on_write = data.disable_2d_canvas_copy_on_write();
  return true;
}

}  // namespace mojo
