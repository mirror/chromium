// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_TEXTURE_SETTINGS_H_
#define COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_TEXTURE_SETTINGS_H_
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/common/resources/resource_format.h"
#include "components/viz/common/resources/resource_settings.h"
#include "components/viz/common/resources/resource_type.h"
#include "components/viz/common/viz_common_export.h"

namespace viz {

// Holds const settings for the resource texture. Never changed after init.
struct VIZ_COMMON_EXPORT ResourceTextureSettings {
  ResourceTextureSettings(ContextProvider* compositor_context_provider,
                          bool delegated_sync_points_needed,
                          const ResourceSettings& resource_settings);

  int max_texture_size = 0;
  bool use_texture_storage = false;
  bool use_texture_format_bgra = false;
  bool use_texture_usage_hint = false;
  bool use_texture_npot = false;
  bool use_sync_query = false;
  bool use_texture_storage_image = false;
  ResourceType default_resource_type = ResourceType::kTexture;
  ResourceFormat yuv_resource_format = LUMINANCE_8;
  ResourceFormat yuv_highbit_resource_format = LUMINANCE_8;
  ResourceFormat best_texture_format = RGBA_8888;
  ResourceFormat best_render_buffer_format = RGBA_8888;
  bool use_gpu_memory_buffer_resources = false;
  bool delegated_sync_points_required = false;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_COMMON_RESOURCES_RESOURCE_TEXTURE_SETTINGS_H_