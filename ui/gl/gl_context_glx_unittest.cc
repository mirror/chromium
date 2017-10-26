// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_context_glx.h"

#include "base/memory/scoped_refptr.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/x/x11_error_tracker.h"
#include "ui/gfx/x/x11_types.h"
#include "ui/gl/gl_surface_glx_x11.h"
#include "ui/gl/init/gl_factory.h"
#include "ui/gl/test/gl_image_test_support.h"

#include <X11/Xlib.h>

namespace gl {

TEST(GLContextGLXTest, DoNotDesrtroyOnFailedMakeCurrent) {
  auto* xdisplay = gfx::GetXDisplay();
  XSynchronize(xdisplay, True);
  XSetWindowAttributes swa;
  memset(&swa, 0, sizeof(swa));
  swa.background_pixmap = 0;
  swa.override_redirect = True;
  auto xwindow = XCreateWindow(xdisplay, DefaultRootWindow(xdisplay), 0, 0, 10,
                               10,              // x, y, width, height
                               0,               // border width
                               CopyFromParent,  // depth
                               InputOutput,
                               CopyFromParent,  // visual
                               CWBackPixmap | CWOverrideRedirect, &swa);
  XMapWindow(xdisplay, xwindow);

  GLImageTestSupport::InitializeGL();
  auto surface =
      gl::InitializeGLSurface(base::MakeRefCounted<GLSurfaceGLXX11>(xwindow));
  scoped_refptr<GLContext> context =
      gl::init::CreateGLContext(nullptr, surface.get(), GLContextAttribs());
  ASSERT_TRUE(context->GetHandle());
  ASSERT_TRUE(context->MakeCurrent(surface.get()));
  EXPECT_TRUE(context->GetHandle());

  context->ReleaseCurrent(surface.get());

  XDestroyWindow(xdisplay, xwindow);
  XSynchronize(xdisplay, False);
  gfx::X11ErrorTracker error_tracker;
  EXPECT_FALSE(context->MakeCurrent(surface.get()));
  ASSERT_TRUE(context->GetHandle());
  EXPECT_TRUE(error_tracker.FoundNewError());
}

}  // namespace gl
