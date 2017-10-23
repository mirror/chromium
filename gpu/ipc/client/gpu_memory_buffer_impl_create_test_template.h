// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file defines tests that implementations of GpuMemoryBufferFactory should
// pass in order to be conformant.

#ifndef GPU_IPC_CLIENT_GPU_MEMORY_BUFFER_IMPL_CREATE_TEST_TEMPLATE_H_
#define GPU_IPC_CLIENT_GPU_MEMORY_BUFFER_IMPL_CREATE_TEST_TEMPLATE_H_

#include "base/bind.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/buffer_format_util.h"

namespace gpu {

template <typename GpuMemoryBufferImplType>
class GpuMemoryBufferImplCreateTest : public testing::Test {};

TYPED_TEST_CASE_P(GpuMemoryBufferImplCreateTest);

TYPED_TEST_P(GpuMemoryBufferImplCreateTest, Create) {
  auto destroyer = [](bool* destroyed, const gpu::SyncToken& sync_token) {
    *destroyed = true;
  };

  const gfx::Size kBufferSize(8, 8);
  const gfx::GpuMemoryBufferId kBufferId(1);
  const gfx::BufferUsage usage = gfx::BufferUsage::SCANOUT;

  for (auto format : gfx::GetBufferFormatsForTesting()) {
    if (!TypeParam::IsConfigurationSupported(format, usage))
      continue;

    bool destroyed = false;

    gfx::GpuMemoryBufferHandle handle;
    std::unique_ptr<TypeParam> buffer(
        TypeParam::Create(kBufferId, kBufferSize, format,
                          base::Bind(destroyer, base::Unretained(&destroyed))));
    ASSERT_TRUE(buffer);
    EXPECT_EQ(buffer->GetFormat(), format);

    // Check if destruction callback is executed when deleting the buffer.
    buffer.reset();
    ASSERT_TRUE(destroyed);
  }
}
// The GpuMemoryBufferImplCreateTest test case verifies behavior that is
// expected from a GpuMemoryBuffer Create() implementation in order to be
// conformant.
REGISTER_TYPED_TEST_CASE_P(GpuMemoryBufferImplCreateTest, Create);

}  // namespace gpu

#endif  // GPU_IPC_CLIENT_GPU_MEMORY_BUFFER_IMPL_CREATE_TEST_TEMPLATE_H_
