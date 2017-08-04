// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/message_loop/message_loop.h"
#include "cc/ipc/frame_sink_id_struct_traits.h"
#include "cc/ipc/local_surface_id_struct_traits.h"
#include "cc/ipc/surface_id_struct_traits.h"
#include "components/viz/common/resources/resource_settings.h"
#include "components/viz/common/surfaces/surface_info.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/viz/public/cpp/compositing/resource_settings_struct_traits.h"
#include "services/viz/public/cpp/compositing/surface_info_struct_traits.h"
#include "services/viz/public/interfaces/compositing/surface_info.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/mojo/geometry_struct_traits.h"

namespace viz {

namespace {

using StructTraitsTest = testing::Test;

TEST_F(StructTraitsTest, ResourceSettings) {
  constexpr size_t kArbitrarySize = 32;
  constexpr bool kArbitraryBool = true;
  ResourceSettings input;
  input.texture_id_allocation_chunk_size = kArbitrarySize;
  input.use_gpu_memory_buffer_resources = kArbitraryBool;
  input.buffer_to_texture_target_map =
      DefaultBufferToTextureTargetMapForTesting();

  ResourceSettings output;
  mojom::ResourceSettings::Deserialize(
      mojom::ResourceSettings::Serialize(&input), &output);
  EXPECT_EQ(input.texture_id_allocation_chunk_size,
            output.texture_id_allocation_chunk_size);
  EXPECT_EQ(input.use_gpu_memory_buffer_resources,
            output.use_gpu_memory_buffer_resources);
  EXPECT_EQ(input.buffer_to_texture_target_map,
            output.buffer_to_texture_target_map);
}

TEST_F(StructTraitsTest, SurfaceInfo) {
  const SurfaceId surface_id(
      FrameSinkId(1234, 4321),
      LocalSurfaceId(5678,
                     base::UnguessableToken::Deserialize(143254, 144132)));
  constexpr float device_scale_factor = 1.234;
  constexpr gfx::Size size(987, 123);

  SurfaceInfo input(surface_id, device_scale_factor, size);
  SurfaceInfo output;
  mojom::SurfaceInfo::Deserialize(mojom::SurfaceInfo::Serialize(&input),
                                  &output);

  EXPECT_EQ(input.id(), output.id());
  EXPECT_EQ(input.size_in_pixels(), output.size_in_pixels());
  EXPECT_EQ(input.device_scale_factor(), output.device_scale_factor());
}

}  // namespace

}  // namespace viz
