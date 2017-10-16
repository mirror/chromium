// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/client/gpu_memory_buffer_impl_dxgi.h"
#include "base/command_line.h"
#include "gpu/ipc/client/gpu_memory_buffer_impl_test_template.h"
#include "ui/gl/gl_switches.h"
#include "ui/gl/test/gl_surface_test_support.h"

namespace gpu {

template <>
class TestDelegate<GpuMemoryBufferImplDXGI> {
 public:
  static void SetUp() {
    // This test only works with hardware rendering.
    DCHECK(base::CommandLine::ForCurrentProcess()->HasSwitch(
        switches::kUseGpuInTests));

    gl::GLSurfaceTestSupport::InitializeOneOff();
  }
};

namespace {

INSTANTIATE_TYPED_TEST_CASE_P(DISABLE_GpuMemoryBufferImplDXGI,
                              GpuMemoryBufferImplTest,
                              GpuMemoryBufferImplDXGI);
}  // namespace
}  // namespace gpu
