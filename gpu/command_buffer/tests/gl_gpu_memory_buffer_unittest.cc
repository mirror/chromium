// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <GLES2/gl2.h>
#include <GLES2/gl2chromium.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2extchromium.h>
#include <stdint.h>

#include <memory>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/memory/ref_counted.h"
#include "base/process/process_handle.h"
#include "gpu/command_buffer/client/gles2_implementation.h"
#include "gpu/command_buffer/service/command_buffer_service.h"
#include "gpu/command_buffer/service/context_group.h"
#include "gpu/command_buffer/service/image_manager.h"
#include "gpu/command_buffer/tests/gl_manager.h"
#include "gpu/command_buffer/tests/gl_test_utils.h"
#include "gpu/ipc/client/gpu_memory_buffer_impl.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/gpu_memory_buffer.h"
#include "ui/gfx/half_float.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_image.h"
#include "ui/gl/gl_switches.h"
#include "ui/gl/init/gl_factory.h"

#if defined(OS_LINUX)
#include <drm.h>
#include <fcntl.h>
#include <gbm.h>
#include <xf86drm.h>
#endif

using testing::_;
using testing::HasSubstr;
using testing::IgnoreResult;
using testing::InvokeWithoutArgs;
using testing::Invoke;
using testing::Return;
using testing::SetArgPointee;
using testing::StrictMock;

namespace gpu {
namespace gles2 {

static const int kImageWidth = 32;
static const int kImageHeight = 32;

class GpuMemoryBufferTest : public testing::TestWithParam<gfx::BufferFormat> {
 protected:
  void SetUp() override {
    GLManager::Options options;
    options.size = gfx::Size(kImageWidth, kImageHeight);
    gl_.Initialize(options);
    gl_.MakeCurrent();
  }

  void TearDown() override {
    gl_.Destroy();
  }

  GLManager gl_;
};

#if defined(OS_LINUX)
void GpuMemoryBufferDeleted(int empty, const gpu::SyncToken& sync_token) {
  // Nothing to do.
}

class GpuMemoryBufferTestEGL : public GpuMemoryBufferTest {
 protected:
  void SetUp() override {
    skip_test_ = false;
    initial_gl_impl_ = gl::GetGLImplementation();

    // TODO(j.isorce): try 0 to N (to handle multi gpu).
    base::FilePath device_path("/dev/dri/card0");
    if (!base::PathExists(device_path)) {
      skip_test_ = true;
      LOG(WARNING) << "Device path does not exist: " << device_path.value();
      return;
    }

    base::ScopedFD device_fd(
        open(device_path.value().c_str(), O_RDWR | O_CLOEXEC));
    ASSERT_TRUE(device_fd.is_valid());

    device_fd_ = std::move(device_fd);

    // In both minigbm and system gbm.
    std::set<std::string> vendor_names = {"amdgpu", "exynos", "i915",
                                          "i915_bpo", "tegra"};

#if defined(USE_SYSTEM_GBM)
    vendor_names.insert("nouveau");
    vendor_names.insert("radeon");
    vendor_names.insert("vc4");
#endif

    drmVersionPtr drm_version = drmGetVersion(device_fd_.get());
    auto vendor_names_itr = vendor_names.find(drm_version->name);
    if (vendor_names_itr == vendor_names.end()) {
      skip_test_ = true;
      LOG(WARNING) << "Not linux drm compatible. Skipping test: "
                   << drm_version->name;
      return;
    }

    if (initial_gl_impl_ != gl::GLImplementation::kGLImplementationEGLGLES2) {
      gl::init::ShutdownGL();
      base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
          switches::kUseGL, gl::kGLImplementationEGLName);
      gl::init::InitializeGLOneOff();
    }

    GpuMemoryBufferTest::SetUp();
    gl_.set_use_native_pixmap_memory_buffers(true);
    info_ = gl_.decoder()->GetContextGroup()->feature_info();
  }

  void TearDown() override {
    if (skip_test_)
      return;

    GpuMemoryBufferTest::TearDown();

    if (initial_gl_impl_ != gl::GLImplementation::kGLImplementationEGLGLES2) {
      gl::init::ShutdownGL();
      gl::init::InitializeGLOneOff();
    }
  }

  bool skip_test_;
  gl::GLImplementation initial_gl_impl_;
  base::ScopedFD device_fd_;
  const FeatureInfo* info_;
};
#endif  // defined(OS_LINUX)

namespace {

#define SHADER(Src) #Src

// clang-format off
const char kVertexShader[] =
SHADER(
  attribute vec4 a_position;
  varying vec2 v_texCoord;
  void main() {
    gl_Position = a_position;
    v_texCoord = vec2((a_position.x + 1.0) * 0.5, (a_position.y + 1.0) * 0.5);
  }
);

const char* kFragmentShader =
SHADER(
  precision mediump float;
  uniform sampler2D a_texture;
  varying vec2 v_texCoord;
  void main() {
    gl_FragColor = texture2D(a_texture, v_texCoord);
  }
);
// clang-format on

void SetRow(gfx::BufferFormat format,
            uint8_t* buffer,
            int width,
            uint8_t pixel[4]) {
  switch (format) {
    case gfx::BufferFormat::R_8:
      for (int i = 0; i < width; ++i)
        buffer[i] = pixel[0];
      return;
    case gfx::BufferFormat::BGR_565:
      for (int i = 0; i < width * 2; i += 2) {
        *reinterpret_cast<uint16_t*>(&buffer[i]) =
            ((pixel[2] >> 3) << 11) | ((pixel[1] >> 2) << 5) | (pixel[0] >> 3);
      }
      return;
    case gfx::BufferFormat::RGBA_4444:
      for (int i = 0; i < width * 2; i += 2) {
        buffer[i + 0] = (pixel[1] << 4) | (pixel[0] & 0xf);
        buffer[i + 1] = (pixel[3] << 4) | (pixel[2] & 0xf);
      }
      return;
    case gfx::BufferFormat::RGBA_8888:
      for (int i = 0; i < width * 4; i += 4) {
        buffer[i + 0] = pixel[0];
        buffer[i + 1] = pixel[1];
        buffer[i + 2] = pixel[2];
        buffer[i + 3] = pixel[3];
      }
      return;
    case gfx::BufferFormat::BGRA_8888:
      for (int i = 0; i < width * 4; i += 4) {
        buffer[i + 0] = pixel[2];
        buffer[i + 1] = pixel[1];
        buffer[i + 2] = pixel[0];
        buffer[i + 3] = pixel[3];
      }
      return;
    case gfx::BufferFormat::RGBA_F16: {
      float float_pixel[4] = {
          pixel[0] / 255.f, pixel[1] / 255.f, pixel[2] / 255.f,
          pixel[3] / 255.f,
      };
      uint16_t half_float_pixel[4];
      gfx::FloatToHalfFloat(float_pixel, half_float_pixel, 4);
      uint16_t* half_float_buffer = reinterpret_cast<uint16_t*>(buffer);
      for (int i = 0; i < width * 4; i += 4) {
        half_float_buffer[i + 0] = half_float_pixel[0];
        half_float_buffer[i + 1] = half_float_pixel[1];
        half_float_buffer[i + 2] = half_float_pixel[2];
        half_float_buffer[i + 3] = half_float_pixel[3];
      }
      return;
    }
    case gfx::BufferFormat::ATC:
    case gfx::BufferFormat::ATCIA:
    case gfx::BufferFormat::BGRX_8888:
    case gfx::BufferFormat::DXT1:
    case gfx::BufferFormat::DXT5:
    case gfx::BufferFormat::ETC1:
    case gfx::BufferFormat::R_16:
    case gfx::BufferFormat::RG_88:
    case gfx::BufferFormat::RGBX_8888:
    case gfx::BufferFormat::UYVY_422:
    case gfx::BufferFormat::YVU_420:
    case gfx::BufferFormat::YUV_420_BIPLANAR:
      NOTREACHED();
      return;
  }

  NOTREACHED();
}

GLenum InternalFormat(gfx::BufferFormat format) {
  switch (format) {
    case gfx::BufferFormat::R_8:
      return GL_RED;
    case gfx::BufferFormat::R_16:
      return GL_R16_EXT;
    case gfx::BufferFormat::RG_88:
      return GL_RG;
    case gfx::BufferFormat::BGR_565:
      return GL_RGB;
    case gfx::BufferFormat::RGBA_4444:
    case gfx::BufferFormat::RGBA_8888:
      return GL_RGBA;
    case gfx::BufferFormat::BGRA_8888:
      return GL_BGRA_EXT;
    case gfx::BufferFormat::RGBA_F16:
      return GL_RGBA;
    case gfx::BufferFormat::ATC:
    case gfx::BufferFormat::ATCIA:
    case gfx::BufferFormat::BGRX_8888:
    case gfx::BufferFormat::DXT1:
    case gfx::BufferFormat::DXT5:
    case gfx::BufferFormat::ETC1:
    case gfx::BufferFormat::RGBX_8888:
    case gfx::BufferFormat::UYVY_422:
    case gfx::BufferFormat::YVU_420:
    case gfx::BufferFormat::YUV_420_BIPLANAR:
      NOTREACHED();
      return 0;
  }

  NOTREACHED();
  return 0;
}

}  // namespace

// An end to end test that tests the whole GpuMemoryBuffer lifecycle.
TEST_P(GpuMemoryBufferTest, Lifecycle) {
  if (GetParam() == gfx::BufferFormat::R_8 &&
      !gl_.GetCapabilities().texture_rg) {
    LOG(WARNING) << "texture_rg not supported. Skipping test.";
    return;
  }

  GLuint texture_id = 0;
  glGenTextures(1, &texture_id);
  ASSERT_NE(0u, texture_id);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  // Create the gpu memory buffer.
  std::unique_ptr<gfx::GpuMemoryBuffer> buffer(gl_.CreateGpuMemoryBuffer(
      gfx::Size(kImageWidth, kImageHeight), GetParam()));

  // Map buffer for writing.
  ASSERT_TRUE(buffer->Map());
  ASSERT_NE(nullptr, buffer->memory(0));
  ASSERT_NE(0, buffer->stride(0));
  uint8_t pixel[] = {255u, 0u, 0u, 255u};

  // Assign a value to each pixel.
  for (int y = 0; y < kImageHeight; ++y) {
    SetRow(GetParam(),
           static_cast<uint8_t*>(buffer->memory(0)) + y * buffer->stride(0),
           kImageWidth, pixel);
  }
  // Unmap the buffer.
  buffer->Unmap();

  // Create the image. This should add the image ID to the ImageManager.
  GLuint image_id =
      glCreateImageCHROMIUM(buffer->AsClientBuffer(), kImageWidth, kImageHeight,
                            InternalFormat(GetParam()));
  ASSERT_NE(0u, image_id);
  ASSERT_TRUE(gl_.decoder()->GetImageManagerForTest()->LookupImage(image_id) !=
              NULL);

  // Bind the image.
  glBindTexImage2DCHROMIUM(GL_TEXTURE_2D, image_id);

  // Build program, buffers and draw the texture.
  GLuint vertex_shader =
      GLTestHelper::LoadShader(GL_VERTEX_SHADER, kVertexShader);
  GLuint fragment_shader =
      GLTestHelper::LoadShader(GL_FRAGMENT_SHADER, kFragmentShader);
  GLuint program = GLTestHelper::SetupProgram(vertex_shader, fragment_shader);
  ASSERT_NE(0u, program);
  glUseProgram(program);

  GLint sampler_location = glGetUniformLocation(program, "a_texture");
  ASSERT_NE(-1, sampler_location);
  glUniform1i(sampler_location, 0);

  GLuint vbo =
      GLTestHelper::SetupUnitQuad(glGetAttribLocation(program, "a_position"));
  ASSERT_NE(0u, vbo);
  glViewport(0, 0, kImageWidth, kImageHeight);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  ASSERT_TRUE(glGetError() == GL_NO_ERROR);

  // Check if pixels match the values that were assigned to the mapped buffer.
  GLTestHelper::CheckPixels(0, 0, kImageWidth, kImageHeight, 0, pixel, nullptr);
  EXPECT_TRUE(GL_NO_ERROR == glGetError());

  // Release the image.
  glReleaseTexImage2DCHROMIUM(GL_TEXTURE_2D, image_id);

  // Clean up.
  glDeleteProgram(program);
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  glDeleteBuffers(1, &vbo);
  glDestroyImageCHROMIUM(image_id);
  glDeleteTextures(1, &texture_id);
}

#if defined(OS_LINUX)
// Test glCreateImageCHROMIUM with gfx::NATIVE_PIXMAP
TEST_F(GpuMemoryBufferTestEGL, NativePixmap) {
  if (skip_test_)
    return;

  ASSERT_EQ(gl::GLImplementation::kGLImplementationEGLGLES2,
            gl::GetGLImplementation());

  EXPECT_TRUE(gl::HasExtension(gl_.context()->GetExtensions(),
                               "EGL_EXT_image_dma_buf_import"));
  EXPECT_THAT(info_->extensions(), HasSubstr("GL_OES_EGL_image"));

  drm_magic_t magic;
  memset(&magic, 0, sizeof(magic));

  // Authenticate.
  int ret = drmGetMagic(device_fd_.get(), &magic) ||
            drmAuthMagic(device_fd_.get(), magic);
  ASSERT_NE(0, ret);

  struct gbm_device* device_handle = gbm_create_device(device_fd_.get());
  ASSERT_NE(nullptr, device_handle);

  struct gbm_bo* gbm_buffer = gbm_bo_create(
      device_handle, kImageWidth, kImageHeight, GBM_FORMAT_ARGB8888,
      GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
  ASSERT_NE(nullptr, gbm_buffer);

  int buffer_size = kImageWidth * kImageHeight * 4;
  std::unique_ptr<uint8_t[]> raw_data{new uint8_t[buffer_size]};
  uint8_t pixel[] = {255u, 0u, 0u, 255u};
  uint32_t stride = gbm_bo_get_stride(gbm_buffer);
  // Assign a value to each pixel.
  for (int y = 0; y < kImageHeight; ++y) {
    SetRow(gfx::BufferFormat::BGRA_8888, raw_data.get() + y * stride,
           kImageWidth, pixel);
  }

  void* map_data = nullptr;
  uint32_t out_stride = 0;
  void* ptr_data = gbm_bo_map(gbm_buffer, 0, 0, gbm_bo_get_width(gbm_buffer),
                              gbm_bo_get_height(gbm_buffer),
                              GBM_BO_TRANSFER_WRITE, &out_stride, &map_data
#if !defined(USE_SYSTEM_GBM)
                              ,
                              0
#endif
                              );

  ASSERT_NE(nullptr, map_data);
  ASSERT_NE(nullptr, ptr_data);

  memcpy(ptr_data, &raw_data[0], buffer_size);
  gbm_bo_unmap(gbm_buffer, map_data);

  base::ScopedFD buffer_fd(gbm_bo_get_fd(gbm_buffer));
  ASSERT_TRUE(buffer_fd.is_valid());

  gfx::GpuMemoryBufferHandle handle;
  handle.type = gfx::NATIVE_PIXMAP;
  handle.native_pixmap_handle.fds.emplace_back(
      base::FileDescriptor(std::move(buffer_fd)));

#if defined(USE_SYSTEM_GBM)
  handle.native_pixmap_handle.planes.emplace_back(stride,
                                                  /* offset */ 0, buffer_size,
                                                  /* modifer */ 0);
#else
  handle.native_pixmap_handle.planes.emplace_back(
      gbm_bo_get_plane_stride(gbm_buffer, 0),
      gbm_bo_get_plane_offset(gbm_buffer, 0),
      gbm_bo_get_plane_size(gbm_buffer, 0),
      gbm_bo_get_plane_format_modifier(gbm_buffer, 0));
#endif

  gfx::BufferFormat format(gfx::BufferFormat::BGRA_8888);
  std::unique_ptr<gfx::GpuMemoryBuffer> buffer =
      gpu::GpuMemoryBufferImpl::CreateFromHandle(
          handle, gfx::Size(kImageWidth, kImageHeight), format,
          gfx::BufferUsage::SCANOUT, base::Bind(&GpuMemoryBufferDeleted, 0));
  ASSERT_NE(nullptr, buffer.get());
  ASSERT_FALSE(buffer->Map());

  // Create the image. This should add the image ID to the ImageManager.
  GLuint image_id = glCreateImageCHROMIUM(buffer->AsClientBuffer(), kImageWidth,
                                          kImageHeight, GL_BGRA_EXT);
  ASSERT_NE(0u, image_id);
  ASSERT_TRUE(gl_.decoder()->GetImageManagerForTest()->LookupImage(image_id) !=
              NULL);

  // Need a texture to bind the image.
  GLuint texture_id = 0;
  glGenTextures(1, &texture_id);
  ASSERT_NE(0u, texture_id);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texture_id);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  // Bind the image.
  glBindTexImage2DCHROMIUM(GL_TEXTURE_2D, image_id);

  // Build program, buffers and draw the texture.
  GLuint vertex_shader =
      GLTestHelper::LoadShader(GL_VERTEX_SHADER, kVertexShader);
  GLuint fragment_shader =
      GLTestHelper::LoadShader(GL_FRAGMENT_SHADER, kFragmentShader);
  GLuint program = GLTestHelper::SetupProgram(vertex_shader, fragment_shader);
  ASSERT_NE(0u, program);
  glUseProgram(program);

  GLint sampler_location = glGetUniformLocation(program, "a_texture");
  ASSERT_NE(-1, sampler_location);
  glUniform1i(sampler_location, 0);

  GLuint vbo =
      GLTestHelper::SetupUnitQuad(glGetAttribLocation(program, "a_position"));
  ASSERT_NE(0u, vbo);
  glViewport(0, 0, kImageWidth, kImageHeight);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  ASSERT_TRUE(glGetError() == GL_NO_ERROR);

  // Check if pixels match the values that were assigned to the mapped buffer.
  GLTestHelper::CheckPixels(0, 0, kImageWidth, kImageHeight, 0, pixel, nullptr);
  EXPECT_TRUE(GL_NO_ERROR == glGetError());

  // Release the image.
  glReleaseTexImage2DCHROMIUM(GL_TEXTURE_2D, image_id);

  // Clean up.
  glDeleteProgram(program);
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);
  glDeleteBuffers(1, &vbo);
  glDestroyImageCHROMIUM(image_id);
  glDeleteTextures(1, &texture_id);
  gbm_bo_destroy(gbm_buffer);
}
#endif  // defined(OS_LINUX)

INSTANTIATE_TEST_CASE_P(GpuMemoryBufferTests,
                        GpuMemoryBufferTest,
                        ::testing::Values(gfx::BufferFormat::R_8,
                                          gfx::BufferFormat::BGR_565,
                                          gfx::BufferFormat::RGBA_4444,
                                          gfx::BufferFormat::RGBA_8888,
                                          gfx::BufferFormat::BGRA_8888,
                                          gfx::BufferFormat::RGBA_F16));

}  // namespace gles2
}  // namespace gpu
