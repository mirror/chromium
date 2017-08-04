// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/message_loop/message_loop.h"
#include "cc/quads/render_pass.h"
#include "components/viz/common/resources/resource_settings.h"
#include "components/viz/common/resources/returned_resource.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/viz/public/cpp/compositing/resource_settings_struct_traits.h"
#include "services/viz/public/cpp/compositing/returned_resource_struct_traits.h"
#include "services/viz/public/interfaces/compositing/returned_resource.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace viz {

namespace {

using StructTraitsTest = testing::Test;

// Test StructTrait serialization and deserialization for copyable type. |input|
// will be serialized and then deserialized into |output|.
template <class MojomType, class Type>
void SerializeAndDeserialize(const Type& input, Type* output) {
  MojomType::Deserialize(MojomType::Serialize(&input), output);
}

// Test StructTrait serialization and deserialization for move only type.
// |input| will be serialized and then deserialized into |output|.
template <class MojomType, class Type>
void SerializeAndDeserialize(Type&& input, Type* output) {
  MojomType::Deserialize(MojomType::Serialize(&input), output);
}

}  // namespace

TEST_F(StructTraitsTest, ResourceSettings) {
  constexpr size_t kArbitrarySize = 32;
  constexpr bool kArbitraryBool = true;
  ResourceSettings input;
  input.texture_id_allocation_chunk_size = kArbitrarySize;
  input.use_gpu_memory_buffer_resources = kArbitraryBool;
  input.buffer_to_texture_target_map =
      DefaultBufferToTextureTargetMapForTesting();

  ResourceSettings output;
  SerializeAndDeserialize<mojom::ResourceSettings>(input, &output);

  EXPECT_EQ(input.texture_id_allocation_chunk_size,
            output.texture_id_allocation_chunk_size);
  EXPECT_EQ(input.use_gpu_memory_buffer_resources,
            output.use_gpu_memory_buffer_resources);
  EXPECT_EQ(input.buffer_to_texture_target_map,
            output.buffer_to_texture_target_map);
}

TEST_F(StructTraitsTest, ReturnedResource) {
  const cc::RenderPassId id = 1337u;
  const gpu::CommandBufferNamespace command_buffer_namespace = gpu::IN_PROCESS;
  const int32_t extra_data_field = 0xbeefbeef;
  const gpu::CommandBufferId command_buffer_id(
      gpu::CommandBufferId::FromUnsafeValue(0xdeadbeef));
  const uint64_t release_count = 0xdeadbeefdead;
  const gpu::SyncToken sync_token(command_buffer_namespace, extra_data_field,
                                  command_buffer_id, release_count);
  const int count = 1234;
  const bool lost = true;

  ReturnedResource input;
  input.id = id;
  input.sync_token = sync_token;
  input.count = count;
  input.lost = lost;

  ReturnedResource output;
  SerializeAndDeserialize<mojom::ReturnedResource>(input, &output);

  EXPECT_EQ(id, output.id);
  EXPECT_EQ(sync_token, output.sync_token);
  EXPECT_EQ(count, output.count);
  EXPECT_EQ(lost, output.lost);
}

}  // namespace viz
