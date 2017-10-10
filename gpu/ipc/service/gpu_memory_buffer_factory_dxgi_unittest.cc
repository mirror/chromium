// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/gpu_memory_buffer_factory_dxgi.h"
#include "gpu/ipc/service/gpu_memory_buffer_factory_test_template.h"

namespace gpu {

namespace {
base::win::ScopedComPtr<ID3D11Device> CreateDevice() {
  D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_1};
  UINT flags = 0;
  D3D_FEATURE_LEVEL feature_level_out = D3D_FEATURE_LEVEL_11_0;
  base::win::ScopedComPtr<ID3D11Device> d3d11_device;
  base::win::ScopedComPtr<ID3D11DeviceContext> d3d11_device_context;
  HRESULT hr = D3D11CreateDevice(
      NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, feature_levels,
      arraysize(feature_levels), D3D11_SDK_VERSION, d3d11_device.GetAddressOf(),
      &feature_level_out, d3d11_device_context.GetAddressOf());
  DCHECK(SUCCEEDED(hr));
  return d3d11_device;
}
}  // namespace

template <>
class TestDelegate<GpuMemoryBufferFactoryDXGI> {
 public:
  static void SetUp() {
    GpuMemoryBufferFactoryDXGI::CreateDeviceTestHook = &CreateDevice;
  }
};

namespace {

INSTANTIATE_TYPED_TEST_CASE_P(DISABLED_GpuMemoryBufferFactoryDXGI,
                              GpuMemoryBufferFactoryTest,
                              GpuMemoryBufferFactoryDXGI);

}  // namespace
}  // namespace gpu
