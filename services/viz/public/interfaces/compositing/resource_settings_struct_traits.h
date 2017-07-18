// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIZ_PUBLIC_INTERFACES_COMPOSITING_RESOURCE_SETTINGS_STRUCT_TRAITS_H_
#define SERVICES_VIZ_PUBLIC_INTERFACES_COMPOSITING_RESOURCE_SETTINGS_STRUCT_TRAITS_H_

#include "components/viz/common/resources/resource_settings.h"
#include "services/viz/public/interfaces/compositing/resource_settings.mojom.h"
#include "ui/gfx/mojo/buffer_types.mojom.h"
#include "ui/gfx/mojo/buffer_types_struct_traits.h"

namespace mojo {

// template <>
// struct StructTraits<viz::mojom::BufferToTextureTargetKeyDataView,
//                     viz::BufferToTextureTargetKey> {
//   static gfx::BufferUsage usage(const viz::BufferToTextureTargetKey& input);

//   static gfx::BufferFormat format(const viz::BufferToTextureTargetKey&
//   input);

//   static bool Read(viz::mojom::BufferToTextureTargetKeyDataView data,
//                    viz::BufferToTextureTargetKey* out);
// }

template <>
struct StructTraits<viz::mojom::ResourceSettingsDataView,
                    viz::ResourceSettings> {
  static uint32_t texture_id_allocation_chunk_size(
      const viz::ResourceSettings& input) {
    return input.texture_id_allocation_chunk_size;
  }

  static bool use_gpu_memory_buffer_resources(
      const viz::ResourceSettings& input) {
    return input.use_gpu_memory_buffer_resources;
  }

  // static const viz::BufferToTextureTargetMap& buffer_to_texture_target_map(
  //     const viz::ResourceSettings& input) {
  //   return input.buffer_to_texture_target_map;
  // }

  static bool Read(viz::mojom::ResourceSettingsDataView data,
                   viz::ResourceSettings* out);
};

}  // namespace mojo

#endif  // SERVICES_VIZ_PUBLIC_INTERFACES_COMPOSITING_RESOURCE_SETTINGS_STRUCT_TRAITS_H_
