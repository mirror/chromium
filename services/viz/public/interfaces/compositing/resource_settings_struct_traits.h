// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_IPC_RESOURCE_SETTINGS_STRUCT_TRAITS_H_
#define CC_IPC_RESOURCE_SETTINGS_STRUCT_TRAITS_H_

#include "components/viz/common/resources/resource_settings.h"
#include "services/viz/public/interfaces/compositing/resource_settings.mojom.h"
#include "ui/gfx/mojo/buffer_types.mojom.h"
#include "ui/gfx/mojo/buffer_types_struct_traits.h"

namespace mojo {

// template <>
// struct StructTraits<cc::mojom::BufferToTextureTargetKeyDataView,
//                     cc::BufferToTextureTargetKey> {
//   static gfx::BufferUsage usage(const cc::BufferToTextureTargetKey& input);

//   static gfx::BufferFormat format(const cc::BufferToTextureTargetKey& input);

//   static bool Read(cc::mojom::BufferToTextureTargetKeyDataView data,
//                    cc::BufferToTextureTargetKey* out);
// }

template <>
struct StructTraits<cc::mojom::ResourceSettingsDataView, cc::ResourceSettings> {
  // static uint32_t texture_id_allocation_chunk_size(
  //     const cc::ResourceSettings& input) {
  //   return input.texture_id_allocation_chunk_size;
  // }

  // static bool use_gpu_memory_buffer_resources(
  //     const cc::ResourceSettings& input) {
  //   return input.use_gpu_memory_buffer_resources;
  // }

  // static const cc::BufferToTextureTargetMap& buffer_to_texture_target_map(
  //     const cc::ResourceSettings& input) {
  //   return input.buffer_to_texture_target_map;
  // }

  static bool Read(cc::mojom::ResourceSettingsDataView data,
                   cc::ResourceSettings* out);
};

}  // namespace mojo

#endif  // CC_IPC_RESOURCE_SETTINGS_STRUCT_TRAITS_H_
