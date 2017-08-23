// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder_unittest.h"

using ::gl::MockGLInterface;
using ::testing::_;
using ::testing::Return;
using ::testing::SetArgPointee;

namespace gpu {
namespace gles2 {

using namespace cmds;

TEST_F(GLES3DecoderPassthroughTest, BindBufferBaseValidArgs) {
  BindBufferBase bind_cmd;
  bind_cmd.Init(GL_TRANSFORM_FEEDBACK_BUFFER, 2, kClientBufferId);
  EXPECT_EQ(error::kNoError, ExecuteCmd(bind_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_F(GLES3DecoderPassthroughTest, BindBufferBaseValidArgsNewId) {
  constexpr GLuint kNewClientId = 502;
  BindBufferBase cmd;
  cmd.Init(GL_TRANSFORM_FEEDBACK_BUFFER, 2, kNewClientId);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
  EXPECT_TRUE(IsObjectHelper<IsBuffer>(kNewClientId));
}

TEST_F(GLES3DecoderPassthroughTest, BindBufferRangeValidArgs) {
  const GLenum kTarget = GL_TRANSFORM_FEEDBACK_BUFFER;
  const GLintptr kRangeOffset = 4;
  const GLsizeiptr kRangeSize = 8;
  const GLsizeiptr kBufferSize = kRangeOffset + kRangeSize;

  cmds::BindBuffer bind_cmd;
  bind_cmd.Init(kTarget, kClientBufferId);
  EXPECT_EQ(error::kNoError, ExecuteCmd(bind_cmd));

  cmds::BufferData buffer_data_cmd;
  buffer_data_cmd.Init(kTarget, kBufferSize, 0, 0, GL_STREAM_DRAW);
  EXPECT_EQ(error::kNoError, ExecuteCmd(buffer_data_cmd));

  BindBufferRange bind_buffer_range_cmd;
  bind_buffer_range_cmd.Init(kTarget, 2, kClientBufferId, kRangeOffset,
                             kRangeSize);
  EXPECT_EQ(error::kNoError, ExecuteCmd(bind_buffer_range_cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_F(GLES3DecoderPassthroughTest, BindBufferRangeValidArgsWithNoData) {
  const GLenum kTarget = GL_TRANSFORM_FEEDBACK_BUFFER;
  const GLintptr kRangeOffset = 4;
  const GLsizeiptr kRangeSize = 8;
  DoBindBuffer(kTarget, kClientBufferId);
  BindBufferRange cmd;
  cmd.Init(kTarget, 2, kClientBufferId, kRangeOffset, kRangeSize);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_F(GLES3DecoderPassthroughTest, BindBufferRangeValidArgsWithLessData) {
  const GLenum kTarget = GL_TRANSFORM_FEEDBACK_BUFFER;
  const GLintptr kRangeOffset = 4;
  const GLsizeiptr kRangeSize = 8;
  const GLsizeiptr kBufferSize = kRangeOffset + kRangeSize - 4;
  DoBindBuffer(kTarget, kClientBufferId);
  DoBufferData(kTarget, kBufferSize, nullptr, GL_STREAM_DRAW);
  BindBufferRange cmd;
  cmd.Init(kTarget, 2, kClientBufferId, kRangeOffset, kRangeSize);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_F(GLES3DecoderPassthroughTest, BindBufferRangeValidArgsNewId) {
  BindBufferRange cmd;
  cmd.Init(GL_TRANSFORM_FEEDBACK_BUFFER, 2, kNewClientId, 4, 4);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
  EXPECT_TRUE(IsObjectHelper<IsBuffer>(kNewClientId));
}

TEST_F(GLES3DecoderPassthroughTest, MapBufferRangeUnmapBufferReadSucceeds) {
  const GLenum kTarget = GL_ARRAY_BUFFER;
  const GLintptr kOffset = 10;
  const GLsizeiptr kSize = 64;
  const GLbitfield kAccess = GL_MAP_READ_BIT;

  uint32_t result_shm_id = shared_memory_id_;
  uint32_t result_shm_offset = kSharedMemoryOffset;
  uint32_t data_shm_id = shared_memory_id_;
  // uint32_t is Result for both MapBufferRange and UnmapBuffer commands.
  uint32_t data_shm_offset = kSharedMemoryOffset + sizeof(uint32_t);

  DoBindBuffer(kTarget, kClientBufferId);
  DoBufferData(kTarget, kSize + kOffset, nullptr, GL_STREAM_DRAW);

  std::vector<int8_t> data(kSize);
  for (GLsizeiptr ii = 0; ii < kSize; ++ii) {
    data[ii] = static_cast<int8_t>(ii % 255);
  }
  DoBufferSubData(kTarget, kOffset, kSize, data.data());

  {  // MapBufferRange
    typedef MapBufferRange::Result Result;
    Result* result = GetSharedMemoryAs<Result*>();

    MapBufferRange cmd;
    cmd.Init(kTarget, kOffset, kSize, kAccess, data_shm_id, data_shm_offset,
             result_shm_id, result_shm_offset);
    *result = 0;
    EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
    int8_t* mem = reinterpret_cast<int8_t*>(&result[1]);
    EXPECT_EQ(0, memcmp(&data[0], mem, kSize));
    EXPECT_EQ(1u, *result);
  }

  {  // UnmapBuffer
    UnmapBuffer cmd;
    cmd.Init(kTarget);
    EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  }

  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

TEST_F(GLES3DecoderPassthroughTest, MapBufferRangeNotInitFails) {
  const GLenum kTarget = GL_ARRAY_BUFFER;
  const GLintptr kOffset = 10;
  const GLsizeiptr kSize = 64;
  const GLbitfield kAccess = GL_MAP_READ_BIT;
  std::vector<int8_t> data(kSize);

  typedef MapBufferRange::Result Result;
  Result* result = GetSharedMemoryAs<Result*>();
  *result = 1;  // Any value other than 0.
  uint32_t result_shm_id = shared_memory_id_;
  uint32_t result_shm_offset = kSharedMemoryOffset;
  uint32_t data_shm_id = shared_memory_id_;
  uint32_t data_shm_offset = kSharedMemoryOffset + sizeof(*result);

  MapBufferRange cmd;
  cmd.Init(kTarget, kOffset, kSize, kAccess, data_shm_id, data_shm_offset,
           result_shm_id, result_shm_offset);
  EXPECT_NE(error::kNoError, ExecuteCmd(cmd));
}

TEST_F(GLES3DecoderPassthroughTest,
       MapBufferRangeWriteInvalidateRangeSucceeds) {
  const GLenum kTarget = GL_ARRAY_BUFFER;
  const GLintptr kOffset = 10;
  const GLsizeiptr kSize = 64;
  // With MAP_INVALIDATE_RANGE_BIT, no need to append MAP_READ_BIT.
  const GLbitfield kAccess = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT;

  DoBindBuffer(kTarget, kClientBufferId);
  DoBufferData(kTarget, kSize, nullptr, GL_STREAM_DRAW);

  typedef MapBufferRange::Result Result;
  Result* result = GetSharedMemoryAs<Result*>();
  *result = 0;
  uint32_t result_shm_id = shared_memory_id_;
  uint32_t result_shm_offset = kSharedMemoryOffset;
  uint32_t data_shm_id = shared_memory_id_;
  uint32_t data_shm_offset = kSharedMemoryOffset + sizeof(*result);

  int8_t* mem = reinterpret_cast<int8_t*>(&result[1]);
  memset(mem, 72, kSize);  // Init to a random value other than 0.

  MapBufferRange cmd;
  cmd.Init(kTarget, kOffset, kSize, kAccess, data_shm_id, data_shm_offset,
           result_shm_id, result_shm_offset);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
}

TEST_F(GLES3DecoderPassthroughTest,
       MapBufferRangeWriteInvalidateBufferSucceeds) {
  // Test INVALIDATE_BUFFER_BIT is mapped to INVALIDATE_RANGE_BIT.
  const GLenum kTarget = GL_ARRAY_BUFFER;
  const GLintptr kOffset = 10;
  const GLsizeiptr kSize = 64;
  const GLbitfield kAccess = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;

  DoBindBuffer(kTarget, kClientBufferId);
  DoBufferData(kTarget, kSize + kOffset, nullptr, GL_STREAM_DRAW);

  typedef MapBufferRange::Result Result;
  Result* result = GetSharedMemoryAs<Result*>();
  *result = 0;
  uint32_t result_shm_id = shared_memory_id_;
  uint32_t result_shm_offset = kSharedMemoryOffset;
  uint32_t data_shm_id = shared_memory_id_;
  uint32_t data_shm_offset = kSharedMemoryOffset + sizeof(*result);

  int8_t* mem = reinterpret_cast<int8_t*>(&result[1]);
  memset(mem, 72, kSize);  // Init to a random value other than 0.

  MapBufferRange cmd;
  cmd.Init(kTarget, kOffset, kSize, kAccess, data_shm_id, data_shm_offset,
           result_shm_id, result_shm_offset);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
}

TEST_F(GLES3DecoderPassthroughTest, MapBufferRangeWithError) {
  const GLenum kTarget = GL_ARRAY_BUFFER;
  const GLintptr kOffset = 10;
  const GLsizeiptr kSize = 64;
  const GLbitfield kAccess = GL_MAP_READ_BIT;
  std::vector<int8_t> data(kSize);
  for (GLsizeiptr ii = 0; ii < kSize; ++ii) {
    data[ii] = static_cast<int8_t>(ii % 255);
  }

  typedef MapBufferRange::Result Result;
  Result* result = GetSharedMemoryAs<Result*>();
  *result = 0;
  uint32_t result_shm_id = shared_memory_id_;
  uint32_t result_shm_offset = kSharedMemoryOffset;
  uint32_t data_shm_id = shared_memory_id_;
  uint32_t data_shm_offset = kSharedMemoryOffset + sizeof(*result);

  int8_t* mem = reinterpret_cast<int8_t*>(&result[1]);
  memset(mem, 72, kSize);  // Init to a random value other than 0.

  MapBufferRange cmd;
  cmd.Init(kTarget, kOffset, kSize, kAccess, data_shm_id, data_shm_offset,
           result_shm_id, result_shm_offset);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  memset(&data[0], 72, kSize);
  // Mem is untouched.
  EXPECT_EQ(0, memcmp(&data[0], mem, kSize));
  EXPECT_EQ(0u, *result);
  EXPECT_EQ(GL_INVALID_OPERATION, GetGLError());
}

TEST_F(GLES3DecoderPassthroughTest, MapBufferRangeBadSharedMemoryFails) {
  const GLenum kTarget = GL_ARRAY_BUFFER;
  const GLintptr kOffset = 10;
  const GLsizeiptr kSize = 64;
  const GLbitfield kAccess = GL_MAP_READ_BIT;

  DoBindBuffer(kTarget, kClientBufferId);
  DoBufferData(kTarget, kOffset + kSize, nullptr, GL_STREAM_DRAW);

  typedef MapBufferRange::Result Result;
  Result* result = GetSharedMemoryAs<Result*>();
  *result = 0;
  uint32_t result_shm_id = shared_memory_id_;
  uint32_t result_shm_offset = kSharedMemoryOffset;
  uint32_t data_shm_id = shared_memory_id_;
  uint32_t data_shm_offset = kSharedMemoryOffset + sizeof(*result);

  MapBufferRange cmd;
  cmd.Init(kTarget, kOffset, kSize, kAccess, kInvalidSharedMemoryId,
           data_shm_offset, result_shm_id, result_shm_offset);
  EXPECT_NE(error::kNoError, ExecuteCmd(cmd));
  cmd.Init(kTarget, kOffset, kSize, kAccess, data_shm_id, data_shm_offset,
           kInvalidSharedMemoryId, result_shm_offset);
  EXPECT_NE(error::kNoError, ExecuteCmd(cmd));
  cmd.Init(kTarget, kOffset, kSize, kAccess, data_shm_id,
           kInvalidSharedMemoryOffset, result_shm_id, result_shm_offset);
  EXPECT_NE(error::kNoError, ExecuteCmd(cmd));
  cmd.Init(kTarget, kOffset, kSize, kAccess, data_shm_id, data_shm_offset,
           result_shm_id, kInvalidSharedMemoryOffset);
  EXPECT_NE(error::kNoError, ExecuteCmd(cmd));
}

TEST_F(GLES3DecoderPassthroughTest, UnmapBufferWriteNotMappedFails) {
  const GLenum kTarget = GL_ARRAY_BUFFER;

  DoBindBuffer(kTarget, kClientBufferId);

  UnmapBuffer cmd;
  cmd.Init(kTarget);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_OPERATION, GetGLError());
}

TEST_F(GLES3DecoderPassthroughTest, UnmapBufferWriteNoBoundBufferFails) {
  const GLenum kTarget = GL_ARRAY_BUFFER;

  UnmapBuffer cmd;
  cmd.Init(kTarget);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_INVALID_OPERATION, GetGLError());
}

TEST_F(GLES3DecoderPassthroughTest, BufferDataDestroysDataStore) {
  const GLenum kTarget = GL_ARRAY_BUFFER;
  const GLintptr kOffset = 10;
  const GLsizeiptr kSize = 64;
  const GLbitfield kAccess = GL_MAP_WRITE_BIT;
  const GLbitfield kFilteredAccess = GL_MAP_WRITE_BIT | GL_MAP_READ_BIT;

  uint32_t result_shm_id = shared_memory_id_;
  uint32_t result_shm_offset = kSharedMemoryOffset;
  uint32_t data_shm_id = shared_memory_id_;
  // uint32_t is Result for both MapBufferRange and UnmapBuffer commands.
  uint32_t data_shm_offset = kSharedMemoryOffset + sizeof(uint32_t);

  DoBindBuffer(kTarget, kClientBufferId);
  DoBufferData(kTarget, kSize + kOffset, nullptr, GL_STREAM_DRAW);

  {  // MapBufferRange succeeds
    typedef MapBufferRange::Result Result;
    Result* result = GetSharedMemoryAs<Result*>();

    MapBufferRange cmd;
    cmd.Init(kTarget, kOffset, kSize, kAccess, data_shm_id, data_shm_offset,
             result_shm_id, result_shm_offset);
    *result = 0;
    EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
    EXPECT_EQ(1u, *result);
  }

  {  // Buffer is tracked as mapped
    PassthroughResources* passthrough_resources = GetPassthroughResources();
    auto mapped_buffer_info_iter =
        passthrough_resources->mapped_buffer_map.find(kClientBufferId);
    EXPECT_NE(mapped_buffer_info_iter,
              passthrough_resources->mapped_buffer_map.end());
    EXPECT_EQ(mapped_buffer_info_iter->second.original_access, kAccess);
    EXPECT_EQ(mapped_buffer_info_iter->second.filtered_access, kFilteredAccess);
  }

  {  // BufferData unmaps the data store.
    DoBufferData(kTarget, kSize * 2, nullptr, GL_STREAM_DRAW);
    EXPECT_EQ(GL_NO_ERROR, GetGLError());
  }

  {  // Buffer is no longer tracked as mapped
    PassthroughResources* passthrough_resources = GetPassthroughResources();
    EXPECT_EQ(passthrough_resources->mapped_buffer_map.find(kClientBufferId),
              passthrough_resources->mapped_buffer_map.end());
  }

  {  // UnmapBuffer fails.
    UnmapBuffer cmd;
    cmd.Init(kTarget);
    EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
    EXPECT_EQ(GL_INVALID_OPERATION, GetGLError());
  }
}

TEST_F(GLES3DecoderPassthroughTest, DeleteBuffersDestroysDataStore) {
  const GLenum kTarget = GL_ARRAY_BUFFER;
  const GLintptr kOffset = 10;
  const GLsizeiptr kSize = 64;
  const GLbitfield kAccess = GL_MAP_WRITE_BIT;
  const GLbitfield kFilteredAccess = GL_MAP_WRITE_BIT | GL_MAP_READ_BIT;

  uint32_t result_shm_id = shared_memory_id_;
  uint32_t result_shm_offset = kSharedMemoryOffset;
  uint32_t data_shm_id = shared_memory_id_;
  // uint32_t is Result for both MapBufferRange and UnmapBuffer commands.
  uint32_t data_shm_offset = kSharedMemoryOffset + sizeof(uint32_t);

  DoBindBuffer(kTarget, kClientBufferId);
  DoBufferData(kTarget, kSize + kOffset, nullptr, GL_STREAM_DRAW);

  {  // MapBufferRange succeeds
    typedef MapBufferRange::Result Result;
    Result* result = GetSharedMemoryAs<Result*>();

    MapBufferRange cmd;
    cmd.Init(kTarget, kOffset, kSize, kAccess, data_shm_id, data_shm_offset,
             result_shm_id, result_shm_offset);
    *result = 0;
    EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
    EXPECT_EQ(1u, *result);
  }

  {  // Buffer is tracked as mapped
    PassthroughResources* passthrough_resources = GetPassthroughResources();
    auto mapped_buffer_info_iter =
        passthrough_resources->mapped_buffer_map.find(kClientBufferId);
    EXPECT_NE(mapped_buffer_info_iter,
              passthrough_resources->mapped_buffer_map.end());
    EXPECT_EQ(mapped_buffer_info_iter->second.original_access, kAccess);
    EXPECT_EQ(mapped_buffer_info_iter->second.filtered_access, kFilteredAccess);
  }

  {  // DeleteBuffers unmaps the data store.
    DoDeleteBuffer(kClientBufferId);
    EXPECT_EQ(GL_NO_ERROR, GetGLError());
  }

  {  // Buffer is no longer tracked as mapped
    PassthroughResources* passthrough_resources = GetPassthroughResources();
    EXPECT_EQ(passthrough_resources->mapped_buffer_map.find(kClientBufferId),
              passthrough_resources->mapped_buffer_map.end());
  }

  {  // UnmapBuffer fails.
    UnmapBuffer cmd;
    cmd.Init(kTarget);
    EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
    EXPECT_EQ(GL_INVALID_OPERATION, GetGLError());
  }
}

TEST_F(GLES3DecoderPassthroughTest, MapUnmapBufferInvalidTarget) {
  const GLenum kTarget = GL_TEXTURE_2D;
  const GLintptr kOffset = 10;
  const GLsizeiptr kSize = 64;
  const GLbitfield kAccess = GL_MAP_WRITE_BIT;

  uint32_t result_shm_id = shared_memory_id_;
  uint32_t result_shm_offset = kSharedMemoryOffset;
  uint32_t data_shm_id = shared_memory_id_;
  // uint32_t is Result for both MapBufferRange and UnmapBuffer commands.
  uint32_t data_shm_offset = kSharedMemoryOffset + sizeof(uint32_t);

  typedef MapBufferRange::Result Result;
  Result* result = GetSharedMemoryAs<Result*>();

  {
    MapBufferRange cmd;
    cmd.Init(kTarget, kOffset, kSize, kAccess, data_shm_id, data_shm_offset,
             result_shm_id, result_shm_offset);
    *result = 0;
    EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
    EXPECT_EQ(0u, *result);
    EXPECT_EQ(GL_INVALID_ENUM, GetGLError());
  }

  {
    UnmapBuffer cmd;
    cmd.Init(kTarget);
    EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
    EXPECT_EQ(GL_INVALID_ENUM, GetGLError());
  }
}

TEST_F(GLES3DecoderPassthroughTest, CopyBufferSubDataValidArgs) {
  const GLenum kTarget = GL_ELEMENT_ARRAY_BUFFER;
  const GLsizeiptr kSize = 64;
  const GLsizeiptr kHalfSize = kSize / 2;
  const GLintptr kReadOffset = 0;
  const GLintptr kWriteOffset = kHalfSize;
  const GLsizeiptr kCopySize = 5;
  const char kValue0 = 3;
  const char kValue1 = 21;

  // Set up the buffer so first half is kValue0 and second half is kValue1.
  DoBindBuffer(kTarget, kClientBufferId);
  DoBufferData(kTarget, kSize, nullptr, GL_STREAM_DRAW);
  std::unique_ptr<char[]> data(new char[kHalfSize]);
  memset(data.get(), kValue0, kHalfSize);
  DoBufferSubData(kTarget, 0, kHalfSize, data.get());
  memset(data.get(), kValue1, kHalfSize);
  DoBufferSubData(kTarget, kHalfSize, kHalfSize, data.get());
  EXPECT_EQ(GL_NO_ERROR, GetGLError());

  cmds::CopyBufferSubData cmd;
  cmd.Init(kTarget, kTarget, kReadOffset, kWriteOffset, kCopySize);
  EXPECT_EQ(error::kNoError, ExecuteCmd(cmd));
  EXPECT_EQ(GL_NO_ERROR, GetGLError());
}

}  // namespace gles2
}  // namespace gpu
