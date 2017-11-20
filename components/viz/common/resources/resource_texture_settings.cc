// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/resources/resource_texture_settings.h"

#include "components/viz/common/resources/platform_color.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "third_party/khronos/GLES2/gl2.h"
#include "third_party/khronos/GLES2/gl2ext.h"

namespace viz {

ResourceTextureSettings::ResourceTextureSettings(
    ContextProvider* compositor_context_provider,
    bool delegated_sync_points_required,
    const ResourceSettings& resource_settings)
    : yuv_highbit_resource_format(
          resource_settings.high_bit_for_testing ? R16_EXT : LUMINANCE_8),
      use_gpu_memory_buffer_resources(
          resource_settings.use_gpu_memory_buffer_resources),
      delegated_sync_points_required(delegated_sync_points_required) {
  if (!compositor_context_provider) {
    // Pick an arbitrary limit here similar to what hardware might.
    max_texture_size = 16 * 1024;
    best_texture_format = RGBA_8888;
    return;
  }

  const auto& caps = compositor_context_provider->ContextCapabilities();
  use_texture_storage = caps.texture_storage;
  use_texture_format_bgra = caps.texture_format_bgra8888;
  use_texture_usage_hint = caps.texture_usage;
  use_texture_npot = caps.texture_npot;
  use_sync_query = caps.sync_query;
  use_texture_storage_image = caps.texture_storage_image;

  if (caps.disable_one_component_textures) {
    yuv_resource_format = yuv_highbit_resource_format = RGBA_8888;
  } else {
    yuv_resource_format = caps.texture_rg ? RED_8 : LUMINANCE_8;
    if (resource_settings.use_r16_texture && caps.texture_norm16)
      yuv_highbit_resource_format = R16_EXT;
    else if (caps.texture_half_float_linear)
      yuv_highbit_resource_format = LUMINANCE_F16;
  }

  gpu::gles2::GLES2Interface* gl = compositor_context_provider->ContextGL();
  gl->GetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);

  best_texture_format =
      PlatformColor::BestSupportedTextureFormat(use_texture_format_bgra);
  best_render_buffer_format = PlatformColor::BestSupportedTextureFormat(
      caps.render_buffer_format_bgra8888);
}

}  // namespace viz
