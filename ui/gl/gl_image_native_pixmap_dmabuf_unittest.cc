// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <drm.h>
#include <gbm.h>

#include "base/files/file_util.h"
#include "base/files/platform_file.h"
#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/linux/native_pixmap_dmabuf.h"
#include "ui/gl/gl_image_native_pixmap.h"
#include "ui/gl/test/gl_image_test_template.h"
#include "ui/gl/test/gl_surface_test_support.h"

namespace gl {

namespace {

class GLImageNativePixmapDambufTest : public testing::Test {
 protected:
  // Overridden from testing::Test:
  void SetUp() override {
    skip_test_ = false;
    device_handle_ = nullptr;

    std::vector<GLImplementation> allowed_impls =
        init::GetAllowedGLImplementations();

    GLImplementation gl_impl = kGLImplementationEGLGLES2;
    bool found = false;
    for (auto impl : allowed_impls) {
      if (impl == gl_impl) {
        found = true;
        break;
      }
    }

    if (!found) {
      skip_test_ = true;
      LOG(WARNING) << "Skip test, egl is required";
      return;
    }

    GLSurfaceTestSupport::InitializeOneOffImplementation(
        gl_impl, /* fallback_to_osmesa */ false);

    const std::string dmabuf_import_ext = "EGL_EXT_image_dma_buf_import";
    std::string platform_extensions(DriverEGL::GetPlatformExtensions());
    ExtensionSet extensions(MakeExtensionSet(platform_extensions));
    if (!HasExtension(extensions, dmabuf_import_ext)) {
      skip_test_ = true;
      LOG(WARNING) << "Skip test, missing extension " << dmabuf_import_ext;
      return;
    }
  }

  void TearDown() override {
    if (device_handle_) {
      gbm_device_destroy(device_handle_);
      device_handle_ = nullptr;
    }
    GLImageTestSupport::CleanupGL();
  }

  void OpenDrmRenderNode() {
    base::FilePath device_path("/dev/dri/renderD128");
    if (!base::PathExists(device_path)) {
      skip_test_ = true;
      LOG(WARNING) << "Device path does not exist: " << device_path.value();
      return;
    }

    base::File drm_file =
        base::File(device_path, base::File::FLAG_OPEN | base::File::FLAG_READ |
                                    base::File::FLAG_WRITE);

    if (!drm_file.IsValid()) {
      skip_test_ = true;
      LOG(WARNING) << "Failed to open render node: " << device_path.value();
      return;
    }

    device_fd_.reset(HANDLE_EINTR(dup(drm_file.GetPlatformFile())));
    if (!device_fd_.is_valid()) {
      skip_test_ = true;
      LOG(WARNING) << "Failed to setup render node: " << device_path.value();
      return;
    }
  }

  void CreateGbmDevice() {
    OpenDrmRenderNode();

    if (skip_test_)
      return;

    device_handle_ = gbm_create_device(device_fd_.get());
    if (!device_handle_) {
      skip_test_ = true;
      LOG(WARNING) << "Failed to create gbm device";
      return;
    }
  }

  bool skip_test_;
  base::ScopedFD device_fd_;
  struct gbm_device* device_handle_;
};

TEST_F(GLImageNativePixmapDambufTest, TestDmabufInvalid) {
  const gfx::Size image_size(64, 64);
  const gfx::BufferFormat format = gfx::BufferFormat::RGBA_8888;

  // Invalid handle.
  gfx::NativePixmapHandle native_pixmap_handle;

  scoped_refptr<gfx::NativePixmap> pixmap(
      new gfx::NativePixmapDmaBuf(image_size, format, native_pixmap_handle));
  EXPECT_NE(nullptr, pixmap);

  scoped_refptr<gl::GLImageNativePixmap> image(new gl::GLImageNativePixmap(
      image_size,
      gl::GLImageNativePixmap::GetInternalFormatForTesting(format)));
  EXPECT_TRUE(image->Initialize(pixmap.get(), pixmap->GetBufferFormat()));

  EXPECT_EQ(image->GetSize().ToString(), image_size.ToString());
}

TEST_F(GLImageNativePixmapDambufTest, TestDmabufValid) {
  CreateGbmDevice();

  if (skip_test_)
    return;

  const gfx::Size image_size(64, 64);
  const gfx::BufferFormat format = gfx::BufferFormat::BGRA_8888;

  struct gbm_bo* gbm_buffer = gbm_bo_create(
      device_handle_, image_size.width(), image_size.height(),
      GBM_FORMAT_ARGB8888, GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
  EXPECT_NE(nullptr, gbm_buffer);

  int buffer_size = image_size.GetArea() * 4;
  uint32_t buffer_stride = gbm_bo_get_stride(gbm_buffer);

  base::ScopedFD buffer_fd(gbm_bo_get_fd(gbm_buffer));
  EXPECT_TRUE(buffer_fd.is_valid());

  gbm_bo_destroy(gbm_buffer);
  gbm_buffer = nullptr;

  gfx::NativePixmapHandle native_pixmap_handle;
  native_pixmap_handle.fds.emplace_back(buffer_fd.release(),
                                        true /* auto_close */);
  native_pixmap_handle.planes.emplace_back(buffer_stride, /* offset */ 0,
                                           buffer_size, /* modifer */ 0);

  scoped_refptr<gfx::NativePixmap> pixmap(
      new gfx::NativePixmapDmaBuf(image_size, format, native_pixmap_handle));
  EXPECT_NE(nullptr, pixmap);

  scoped_refptr<gl::GLImageNativePixmap> image(new gl::GLImageNativePixmap(
      image_size,
      gl::GLImageNativePixmap::GetInternalFormatForTesting(format)));
  EXPECT_TRUE(image->Initialize(pixmap.get(), pixmap->GetBufferFormat()));

  EXPECT_EQ(image->GetSize().ToString(), image_size.ToString());
}

}  // namespace

}  // namespace gl
