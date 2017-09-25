// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/buffer_format_util.h"
#include "ui/gl/gl_angle_util_win.h"
#include "ui/gl/gl_image_dxgi.h"
#include "ui/gl/test/gl_image_test_template.h"

namespace gl {
namespace {

const uint8_t kRGBImageColor[] = {0x30, 0x40, 0x10, 0xFF};

template <gfx::BufferFormat format>
class GLImageDXGITestDelegate {
 public:
  scoped_refptr<GLImage> CreateSolidColorImage(const gfx::Size& size,
                                               const uint8_t color[4]) const {
    D3D11_TEXTURE2D_DESC desc;
    desc.Width = size.width();
    desc.Height = size.height();
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // todo: based on format?
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_NTHANDLE |
                     D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

    std::vector<unsigned char> originalData(size.width() * size.height() * 4);
    for (int x = 0; x < size.width(); ++x) {
      for (int y = 0; y < size.height(); ++y) {
        for (int i = 0; i < 4; ++i) {
          originalData[(x + y * size.width()) * 4 + i] = color[i];
        }
      }
    }
    D3D11_SUBRESOURCE_DATA initial_data = {};
    initial_data.pSysMem = originalData.data();
    initial_data.SysMemPitch = size.width() * 4;

    base::win::ScopedComPtr<ID3D11Device> d3d11_device =
        QueryD3D11DeviceObjectFromANGLE();

    base::win::ScopedComPtr<ID3D11Texture2D> texture;
    HRESULT hr = d3d11_device->CreateTexture2D(&desc, &initial_data,
                                               texture.GetAddressOf());
    EXPECT_TRUE(SUCCEEDED(hr));

    base::win::ScopedComPtr<IDXGIResource1> dxgi_resource;
    hr = texture.CopyTo(dxgi_resource.GetAddressOf());
    EXPECT_TRUE(SUCCEEDED(hr));

    HANDLE handle = nullptr;
    hr = dxgi_resource->CreateSharedHandle(
        nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE,
        nullptr, &handle);
    EXPECT_TRUE(SUCCEEDED(hr));

    scoped_refptr<GLImageDXGIHandle> image(new GLImageDXGIHandle(
        size, base::win::ScopedHandle(handle), 0, format));
    bool rv = image->Initialize();

    EXPECT_TRUE(rv);
    return image;
  }

  scoped_refptr<GLImage> CreateImage(const gfx::Size& size) const {
    const uint8_t color[4] = {0, 0, 0, 0};
    return CreateSolidColorImage(size, color);
  }

  unsigned GetTextureTarget() const { return GL_TEXTURE_2D; }
  const uint8_t* GetImageColor() { return kRGBImageColor; }
};

using GLImageTestTypes =
    testing::Types<GLImageDXGITestDelegate<gfx::BufferFormat::RGBA_8888>,
                   GLImageDXGITestDelegate<gfx::BufferFormat::RGBX_8888>>;

INSTANTIATE_TYPED_TEST_CASE_P(GLImageDXGIHandle, GLImageTest, GLImageTestTypes);

using GLImageRGBTestTypes =
    testing::Types<GLImageDXGITestDelegate<gfx::BufferFormat::RGBA_8888>,
                   GLImageDXGITestDelegate<gfx::BufferFormat::RGBX_8888>>;

INSTANTIATE_TYPED_TEST_CASE_P(GLImageDXGIHandle,
                              GLImageZeroInitializeTest,
                              GLImageRGBTestTypes);

INSTANTIATE_TYPED_TEST_CASE_P(
    GLImageDXGIHandle,
    GLImageBindTest,
    GLImageDXGITestDelegate<gfx::BufferFormat::RGBA_8888>);

}  // namespace
}  // namespace gl
