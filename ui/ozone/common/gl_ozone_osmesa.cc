// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/common/gl_ozone_osmesa.h"

#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_context_osmesa.h"
#include "ui/gl/gl_gl_api_implementation.h"
#include "ui/gl/gl_implementation_osmesa.h"
#include "ui/gl/gl_osmesa_api_implementation.h"
#include "ui/gl/gl_share_group.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/gl_surface_format.h"
#include "ui/gl/gl_surface_osmesa.h"

namespace ui {

GLOzoneOSMesa::GLOzoneOSMesa() {}

GLOzoneOSMesa::~GLOzoneOSMesa() {}

bool GLOzoneOSMesa::InitializeGLOneOffPlatform() {
  return true;
}

bool GLOzoneOSMesa::InitializeStaticGLBindings(
    gl::GLImplementation implementation) {
#if defined(OS_FUCHSIA)
  // KM: PUNT! I have no idea what I'm doing WRT graphics.
  return false;
#else
  return gl::InitializeStaticGLBindingsOSMesaGL();
#endif
}

void GLOzoneOSMesa::InitializeDebugGLBindings() {
  // Also punt
#if !defined(OS_FUCHSIA)
  gl::InitializeDebugGLBindingsGL();
  gl::InitializeDebugGLBindingsOSMESA();
#endif
}

void GLOzoneOSMesa::ShutdownGL() {
#if !defined(OS_FUCHSIA)
  gl::ClearBindingsGL();
  gl::ClearBindingsOSMESA();
#endif
}

bool GLOzoneOSMesa::GetGLWindowSystemBindingInfo(
    gl::GLWindowSystemBindingInfo* info) {
  return false;
}

scoped_refptr<gl::GLContext> GLOzoneOSMesa::CreateGLContext(
    gl::GLShareGroup* share_group,
    gl::GLSurface* compatible_surface,
    const gl::GLContextAttribs& attribs) {
  return gl::InitializeGLContext(new gl::GLContextOSMesa(share_group),
                                 compatible_surface, attribs);
}

scoped_refptr<gl::GLSurface> GLOzoneOSMesa::CreateViewGLSurface(
    gfx::AcceleratedWidget window) {
  return gl::InitializeGLSurface(new gl::GLSurfaceOSMesaHeadless());
}

scoped_refptr<gl::GLSurface> GLOzoneOSMesa::CreateSurfacelessViewGLSurface(
    gfx::AcceleratedWidget window) {
  return nullptr;
}

scoped_refptr<gl::GLSurface> GLOzoneOSMesa::CreateOffscreenGLSurface(
    const gfx::Size& size) {
  return gl::InitializeGLSurface(
      new gl::GLSurfaceOSMesa(gl::GLSurfaceFormat::PIXEL_LAYOUT_BGRA, size));
}

}  // namespace ui
