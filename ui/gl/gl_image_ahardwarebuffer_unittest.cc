// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <vector>

#include "base/android/android_hardware_buffer_compat.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gl/gl_image_ahardwarebuffer.h"
#include "ui/gl/test/gl_image_test_template.h"

namespace gl {
namespace {

const uint8_t kGreen[] = {0x0, 0xFF, 0x0, 0xFF};

template <gfx::BufferFormat format>
class GLImageAHardwareBufferTestDelegate {
 public:
  scoped_refptr<GLImage> CreateSolidColorImage(const gfx::Size& size,
                                               const uint8_t color[4]) const {
    if (!base::AndroidHardwareBufferCompat::IsSupportAvailable()) {
      return nullptr;
    }

    // AHardwareBuffer_Desc's stride parameter is in pixels, not in bytes.
    //
    // We can't use the 3-byte-per-pixel AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM
    // format in this test since there's no matching gfx::BufferFormat,
    // gfx::BufferFormat::RGBX_8888 assumes 4 bytes per pixel.
    const int pixel_bytes = 4;

    // cf. gpu_memory_buffer_impl_android_hardware_buffer
    AHardwareBuffer_Desc desc = {};
    desc.width = size.width();
    desc.height = size.height();
    desc.layers = 1;  // number of images
    desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM;
    desc.usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
                 AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT |
                 AHARDWAREBUFFER_USAGE_CPU_READ_RARELY |
                 AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY;

    AHardwareBuffer* buffer = nullptr;
    base::AndroidHardwareBufferCompat::GetInstance().Allocate(&desc, &buffer);
    EXPECT_TRUE(buffer);

    uint8_t* data = nullptr;
    int ret;
    ret = base::AndroidHardwareBufferCompat::GetInstance().Lock(
        buffer, AHARDWAREBUFFER_USAGE_CPU_WRITE_RARELY, -1, nullptr,
        (void**)(&data));
    EXPECT_EQ(ret, 0);
    EXPECT_TRUE(data);

    AHardwareBuffer_Desc desc_locked = {};
    base::AndroidHardwareBufferCompat::GetInstance().Describe(buffer,
                                                              &desc_locked);

    GLImageTestSupport::SetBufferDataToColor(size.width(), size.height(),
                                             desc_locked.stride * pixel_bytes,
                                             0, format, color, data);

    ret = base::AndroidHardwareBufferCompat::GetInstance().Unlock(buffer,
                                                                  nullptr);
    EXPECT_EQ(ret, 0);

    scoped_refptr<GLImageAHardwareBuffer> image(
        new GLImageAHardwareBuffer(size));
    EGLint attribs[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};
    EGLClientBuffer client_buffer = eglGetNativeClientBufferANDROID(buffer);
    bool rv =
        image->Initialize(EGL_NATIVE_BUFFER_ANDROID, client_buffer, attribs);
    EXPECT_TRUE(rv);
    return image;
  }

  scoped_refptr<GLImage> CreateImage(const gfx::Size& size) const {
    const uint8_t color[4] = {0, 0, 0, 0};
    return CreateSolidColorImage(size, color);
  }

  unsigned GetTextureTarget() const { return GL_TEXTURE_2D; }
  const uint8_t* GetImageColor() { return kGreen; }
  int GetAdmissibleError() const { return 0; }
};

using GLImageTestTypes = testing::Types<
    GLImageAHardwareBufferTestDelegate<gfx::BufferFormat::RGBA_8888>>;

// Disable the tests by default for now since they require Android O,
// the test bots don't generally have that yet. For manual testing,
// add: --gtest_also_run_disabled_tests -f 'GLImageAHardwareBuffer/*'

INSTANTIATE_TYPED_TEST_CASE_P(DISABLED_GLImageAHardwareBuffer,
                              GLImageTest,
                              GLImageTestTypes);

INSTANTIATE_TYPED_TEST_CASE_P(DISABLED_GLImageAHardwareBuffer,
                              GLImageOddSizeTest,
                              GLImageTestTypes);

INSTANTIATE_TYPED_TEST_CASE_P(DISABLED_GLImageAHardwareBuffer,
                              GLImageBindTest,
                              GLImageTestTypes);

INSTANTIATE_TYPED_TEST_CASE_P(DISABLED_GLImageAHardwareBuffer,
                              GLImageZeroInitializeTest,
                              GLImageTestTypes);

}  // namespace
}  // namespace gl
