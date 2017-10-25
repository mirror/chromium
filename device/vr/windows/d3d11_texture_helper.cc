// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/windows/d3d11_texture_helper.h"
#include "mojo/public/c/system/platform_handle.h"

namespace {
#include "device/vr/windows/flip_pixel_shader.h"
#include "device/vr/windows/flip_vertex_shader.h"
}

namespace device {

D3D11TextureHelper::D3D11TextureHelper() {}

D3D11TextureHelper::~D3D11TextureHelper() {}

bool D3D11TextureHelper::CopyTextureToBackBuffer(bool flipY) {
  if (!EnsureInitialized())
    return false;
  if (!source_texture_)
    return false;
  if (!target_texture_)
    return false;

  HRESULT hr = keyed_mutex_->AcquireSync(1, 2000);
  if (FAILED(hr) || hr == WAIT_TIMEOUT || hr == WAIT_ABANDONED) {
    // Failure will self-correct on release builds.
    NOTREACHED();
    return false;
  }

  if (!flipY) {
    d3d11_device_context_->CopyResource(target_texture_.Get(),
                                        source_texture_.Get());
  } else {
    if (!render_target_view_) {
      D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
      render_target_view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
      render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
      render_target_view_desc.Texture2D.MipSlice = 0;
      hr = d3d11_device_->CreateRenderTargetView(target_texture_.Get(),
                                                 &render_target_view_desc,
                                                 &render_target_view_);
      if (FAILED(hr))
        return false;
      d3d11_device_context_->OMSetRenderTargets(
          1, render_target_view_.GetAddressOf(), nullptr);
    }

    if (!flip_vertex_shader_) {
      hr = d3d11_device_->CreateVertexShader(g_flip_vertex,
                                             _countof(g_flip_vertex), nullptr,
                                             &flip_vertex_shader_);
      if (FAILED(hr))
        return false;
      hr = d3d11_device_->CreatePixelShader(
          g_flip_pixel, _countof(g_flip_pixel), nullptr, &flip_pixel_shader_);
      if (FAILED(hr))
        return false;
      d3d11_device_context_->VSSetShader(flip_vertex_shader_.Get(), nullptr, 0);
      d3d11_device_context_->PSSetShader(flip_pixel_shader_.Get(), nullptr, 0);
      D3D11_INPUT_ELEMENT_DESC vertex_desc = {
          "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,
          0,          0, D3D11_INPUT_PER_VERTEX_DATA,
          0};
      hr = d3d11_device_->CreateInputLayout(&vertex_desc, 1, g_flip_vertex,
                                            _countof(g_flip_vertex),
                                            &input_layout_);
      if (FAILED(hr))
        return false;
      d3d11_device_context_->IASetInputLayout(input_layout_.Get());
    }

    if (!vertex_buffer_) {
      UINT stride = 2 * sizeof(float);
      UINT offset = 0;
      float buffer_data[] = {-1, -1, -1, 1, 1, -1, -1, 1, 1, 1, 1, -1};
      D3D11_SUBRESOURCE_DATA vertex_buffer_data;
      vertex_buffer_data.pSysMem = buffer_data;
      vertex_buffer_data.SysMemPitch = 0;
      vertex_buffer_data.SysMemSlicePitch = 0;
      CD3D11_BUFFER_DESC vertex_buffer_desc(sizeof(buffer_data),
                                            D3D11_BIND_VERTEX_BUFFER);
      hr = d3d11_device_->CreateBuffer(&vertex_buffer_desc, &vertex_buffer_data,
                                       &vertex_buffer_);
      if (FAILED(hr))
        return false;
      d3d11_device_context_->IASetVertexBuffers(
          0, 1, vertex_buffer_.GetAddressOf(), &stride, &offset);
    }

    if (!sampler_) {
      CD3D11_DEFAULT default_values;
      CD3D11_SAMPLER_DESC sampler_desc = CD3D11_SAMPLER_DESC(default_values);
      D3D11_SAMPLER_DESC sd = sampler_desc;
      hr = d3d11_device_->CreateSamplerState(&sd, sampler_.GetAddressOf());
      if (FAILED(hr))
        return false;
      d3d11_device_context_->PSSetSamplers(0, 1, sampler_.GetAddressOf());
    }

    // Always create new shader resource view, since input texture may change.
    D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc;
    shader_resource_view_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    shader_resource_view_desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
    shader_resource_view_desc.Texture2D.MostDetailedMip = 0;
    shader_resource_view_desc.Texture2D.MipLevels = 1;
    hr = d3d11_device_->CreateShaderResourceView(
        source_texture_.Get(), &shader_resource_view_desc,
        shader_resource_.ReleaseAndGetAddressOf());
    if (FAILED(hr))
      return false;
    d3d11_device_context_->PSSetShaderResources(
        0, 1, shader_resource_.GetAddressOf());

    D3D11_TEXTURE2D_DESC desc;
    target_texture_->GetDesc(&desc);
    D3D11_VIEWPORT viewport = {0, 0, desc.Width, desc.Height, 0, 1};
    d3d11_device_context_->RSSetViewports(1, &viewport);
    d3d11_device_context_->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    d3d11_device_context_->Draw(6, 0);
  }

  keyed_mutex_->ReleaseSync(0);
  return true;
}

void D3D11TextureHelper::SetSourceTexture(mojo::ScopedHandle texture_handle) {
  source_texture_ = nullptr;
  keyed_mutex_ = nullptr;

  MojoPlatformHandle platform_handle;
  platform_handle.struct_size = sizeof(platform_handle);
  MojoResult result = MojoUnwrapPlatformHandle(texture_handle.release().value(),
                                               &platform_handle);
  if (result != MOJO_RESULT_OK)
    return;
  HANDLE handle = reinterpret_cast<HANDLE>(platform_handle.value);
  SetSourceTexture(base::win::ScopedHandle(handle));
}

void D3D11TextureHelper::SetSourceTexture(
    base::win::ScopedHandle texture_handle) {
  source_texture_ = nullptr;
  keyed_mutex_ = nullptr;

  if (!EnsureInitialized())
    return;
  texture_handle_ = std::move(texture_handle);
  HRESULT hr = d3d11_device_->OpenSharedResource1(
      texture_handle_.Get(),
      IID_PPV_ARGS(keyed_mutex_.ReleaseAndGetAddressOf()));
  if (FAILED(hr))
    return;
  hr = keyed_mutex_.CopyTo(source_texture_.ReleaseAndGetAddressOf());
  if (FAILED(hr)) {
    keyed_mutex_ = nullptr;
  }
}

void D3D11TextureHelper::AllocateBackBuffer() {
  if (!EnsureInitialized())
    return;
  if (!source_texture_)
    return;

  D3D11_TEXTURE2D_DESC desc_source;
  source_texture_->GetDesc(&desc_source);
  desc_source.MiscFlags = 0;

  if (target_texture_) {
    D3D11_TEXTURE2D_DESC desc_target;
    target_texture_->GetDesc(&desc_target);
    if (desc_source.Width != desc_target.Width ||
        desc_source.Height != desc_target.Height ||
        desc_source.MipLevels != desc_target.MipLevels ||
        desc_source.ArraySize != desc_target.ArraySize ||
        desc_source.Format != desc_target.Format ||
        desc_source.SampleDesc.Count != desc_target.SampleDesc.Count ||
        desc_source.SampleDesc.Quality != desc_target.SampleDesc.Quality ||
        desc_source.Usage != desc_target.Usage ||
        desc_source.BindFlags != desc_target.BindFlags ||
        desc_source.CPUAccessFlags != desc_target.CPUAccessFlags ||
        desc_source.MiscFlags != desc_target.MiscFlags) {
      target_texture_ = nullptr;
    }
  }

  if (!target_texture_) {
    // Ignoring error - target_texture_ will be null on failure.
    d3d11_device_->CreateTexture2D(&desc_source, nullptr,
                                   target_texture_.ReleaseAndGetAddressOf());
  }
}

Microsoft::WRL::ComPtr<ID3D11Texture2D> D3D11TextureHelper::GetBackbuffer() {
  return target_texture_;
}

void D3D11TextureHelper::SetBackbuffer(
    Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer) {
  target_texture_ = back_buffer;
}

bool D3D11TextureHelper::EnsureInitialized() {
  if (d3d11_device_ && SUCCEEDED(d3d11_device_->GetDeviceRemovedReason()))
    return true;  // Already initialized.

  // If we were previously initialized, but lost the device, throw away old
  // state.  This will be initialized lazily as needed.
  d3d11_device_ = nullptr;
  d3d11_device_context_ = nullptr;
  render_target_view_ = nullptr;
  flip_pixel_shader_ = nullptr;
  flip_vertex_shader_ = nullptr;
  input_layout_ = nullptr;
  vertex_buffer_ = nullptr;
  sampler_ = nullptr;
  shader_resource_ = nullptr;
  target_texture_ = nullptr;
  source_texture_ = nullptr;
  keyed_mutex_ = nullptr;

  D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_1};
  UINT flags = 0;
  D3D_FEATURE_LEVEL feature_level_out = D3D_FEATURE_LEVEL_11_1;
#if defined _DEBUG
  flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

  Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device;
  HRESULT hr = D3D11CreateDevice(
      NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, feature_levels,
      arraysize(feature_levels), D3D11_SDK_VERSION, d3d11_device.GetAddressOf(),
      &feature_level_out, d3d11_device_context_.GetAddressOf());
  if (SUCCEEDED(hr)) {
    hr = d3d11_device.As(&d3d11_device_);
    if (FAILED(hr)) {
      d3d11_device_context_ = nullptr;
    }
  }
  return SUCCEEDED(hr);
}

}  // namespace device
