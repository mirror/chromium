// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/message_loop/message_loop.h"
#include "cc/ipc/frame_sink_id_struct_traits.h"
#include "components/viz/common/resources/resource_settings.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/viz/public/cpp/compositing/resource_settings_struct_traits.h"
#include "services/viz/public/cpp/compositing/surface_sequence_struct_traits.h"
#include "services/viz/public/interfaces/compositing/surface_sequence.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

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

TEST_F(StructTraitsTest, SurfaceSequence) {
  const FrameSinkId frame_sink_id(2016, 1234);
  const uint32_t sequence = 0xfbadbeef;

  SurfaceSequence input(frame_sink_id, sequence);
  SurfaceSequence output;
  mojom::SurfaceSequence::Deserialize(mojom::SurfaceSequence::Serialize(&input),
                                      &output);

  EXPECT_EQ(frame_sink_id, output.frame_sink_id);
  EXPECT_EQ(sequence, output.sequence);
}

}  // namespace

}  // namespace viz
