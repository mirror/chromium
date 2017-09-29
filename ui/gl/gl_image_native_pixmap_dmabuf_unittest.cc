// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <drm.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "base/files/file_util.h"
#include "base/files/platform_file.h"
#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/linux/native_pixmap_dmabuf.h"
#include "ui/gfx/x/x11_types.h"
#include "ui/gl/gl_image_native_pixmap.h"
#include "ui/gl/test/gl_image_test_template.h"
#include "ui/gl/test/gl_surface_test_support.h"

#include <X11/Xlib-xcb.h>
#include <xcb/dri2.h>

namespace gl {

namespace {

static xcb_screen_t* get_xcb_screen(xcb_screen_iterator_t iter, int screen) {
  for (; iter.rem; --screen, xcb_screen_next(&iter))
    if (screen == 0)
      return iter.data;

  return NULL;
}

class GLImageNativePixmapDambufTest : public testing::Test {
 protected:
  // Overridden from testing::Test:
  void SetUp() override {
    skip_test_ = false;
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

    base::FilePath device_path("/dev/dri/card0");
    if (!base::PathExists(device_path)) {
      skip_test_ = true;
      LOG(WARNING) << "Device path does not exist: " << device_path.value();
      return;
    }

    base::ScopedFD device_fd(
        open(device_path.value().c_str(), O_RDWR | O_CLOEXEC));
    EXPECT_TRUE(device_fd.is_valid());

    drm_auth_t auth;
    memset(&auth, 0, sizeof(auth));
    EXPECT_EQ(0, ioctl(device_fd.get(), DRM_IOCTL_GET_MAGIC, &auth));

    xcb_connection_t* xcb_conn = XGetXCBConnection(gfx::GetXDisplay());
    xcb_screen_iterator_t s = xcb_setup_roots_iterator(xcb_get_setup(xcb_conn));
    xcb_screen_t* xcb_screen =
        get_xcb_screen(s, XDefaultScreen(gfx::GetXDisplay()));
    xcb_dri2_authenticate_cookie_t authenticate_cookie =
        xcb_dri2_authenticate_unchecked(xcb_conn, xcb_screen->root, auth.magic);
    xcb_dri2_authenticate_reply_t* authenticate =
        xcb_dri2_authenticate_reply(xcb_conn, authenticate_cookie, NULL);
    EXPECT_TRUE(authenticate);
    EXPECT_TRUE(authenticate->authenticated);

    device_fd_ = std::move(device_fd);
  }

  void TearDown() override { GLImageTestSupport::CleanupGL(); }

  bool skip_test_;
  base::ScopedFD device_fd_;
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
  if (skip_test_)
    return;

  const gfx::Size image_size(64, 64);
  const gfx::BufferFormat format = gfx::BufferFormat::RGBA_8888;
  const int bpp = 32;

  struct drm_mode_create_dumb request;
  memset(&request, 0, sizeof(request));
  request.width = image_size.width();
  request.height = image_size.height();
  request.bpp = bpp;
  request.flags = 0;

  EXPECT_TRUE(ioctl(device_fd_.get(), DRM_IOCTL_MODE_CREATE_DUMB, &request) >=
              0);

  struct drm_prime_handle args;
  memset(&args, 0, sizeof(args));
  args.fd = base::kInvalidPlatformFile;
  args.handle = request.handle;
  args.flags = O_RDONLY | O_CLOEXEC;

  EXPECT_TRUE(ioctl(device_fd_.get(), DRM_IOCTL_PRIME_HANDLE_TO_FD, &args) >=
              0);

  base::ScopedFD scoped_fd(HANDLE_EINTR(dup(args.fd)));
  EXPECT_TRUE(scoped_fd.is_valid());

  gfx::NativePixmapHandle native_pixmap_handle;
  native_pixmap_handle.fds.emplace_back(scoped_fd.release(),
                                        true /* auto_close */);
  native_pixmap_handle.planes.emplace_back(request.pitch, /* offset */ 0,
                                           request.size, /* modifer */ 0);

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
