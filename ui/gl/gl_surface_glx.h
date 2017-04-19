// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GL_GL_SURFACE_GLX_H_
#define UI_GL_GL_SURFACE_GLX_H_

#include <stdint.h>
extern "C" {
#include <X11/Xlib.h>
}

#include <memory>
#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/vsync_provider.h"
#include "ui/gfx/x/x11_types.h"
#include "ui/gl/gl_export.h"
#include "ui/gl/gl_surface.h"

namespace gl {

// Base class for GLX surfaces.
class GL_EXPORT GLSurfaceGLX : public GLSurface {
 public:
  GLSurfaceGLX();

  static bool InitializeOneOff();

  // These aren't particularly tied to surfaces, but since we already
  // have the static InitializeOneOff here, it's easiest to reuse its
  // initialization guards.
  static const char* GetGLXExtensions();
  static bool HasGLXExtension(const char* name);
  static bool IsCreateContextSupported();
  static bool IsCreateContextRobustnessSupported();
  static bool IsCreateContextProfileSupported();
  static bool IsCreateContextES2ProfileSupported();
  static bool IsTextureFromPixmapSupported();
  static bool IsOMLSyncControlSupported();
  static bool IsEXTSwapControlSupported();
  static bool IsMESASwapControlSupported();

  void* GetDisplay() override;

  // Get the FB config that the surface was created with or NULL if it is not
  // a GLX drawable.
  void* GetConfig() override = 0;

  unsigned long GetCompatibilityKey() override = 0;

 protected:
  ~GLSurfaceGLX() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(GLSurfaceGLX);
};

// A surface used to render to a view.
class GL_EXPORT NativeViewGLSurfaceGLX : public GLSurfaceGLX {
 public:
  explicit NativeViewGLSurfaceGLX(gfx::AcceleratedWidget window);

  // Implement GLSurfaceGLX.
  bool Initialize(GLSurfaceFormat format) override;
  void Destroy() override;
  bool Resize(const gfx::Size& size,
              float scale_factor,
              bool has_alpha) override;
  bool IsOffscreen() override;
  gfx::SwapResult SwapBuffers() override;
  gfx::Size GetSize() override;
  void* GetHandle() override;
  bool SupportsPostSubBuffer() override;
  void* GetConfig() override;
  GLSurfaceFormat GetFormat() override;
  unsigned long GetCompatibilityKey() override;
  gfx::SwapResult PostSubBuffer(int x, int y, int width, int height) override;
  gfx::VSyncProvider* GetVSyncProvider() override;

  VisualID GetVisualID() const { return visual_id_; }

 protected:
  ~NativeViewGLSurfaceGLX() override;

  gfx::AcceleratedWidget window() const { return window_; }

  // Handle registering and unregistering for Expose events.
  virtual void RegisterEvents() = 0;
  virtual void UnregisterEvents() = 0;

  // Forwards Expose event to child window.
  void ForwardExposeEvent(XEvent* xevent);

  // Checks if event is Expose for child window.
  bool CanHandleEvent(XEvent* xevent);

 private:
  // The handle for the drawable to make current or swap.
  GLXDrawable GetDrawableHandle() const;

  // Window passed in at creation. Always valid.
  const gfx::AcceleratedWidget parent_window_;

  // Child window, used to control resizes so that they're in-order with GL.
  gfx::AcceleratedWidget window_ = 0;

  // GLXDrawable for the window.
  GLXWindow glx_window_ = 0;

  GLXFBConfig config_ = nullptr;
  gfx::Size size_;
  VisualID visual_id_ = CopyFromParent;

  std::unique_ptr<gfx::VSyncProvider> vsync_provider_;

  DISALLOW_COPY_AND_ASSIGN(NativeViewGLSurfaceGLX);
};

// A surface used to render to an offscreen pbuffer.
class GL_EXPORT UnmappedNativeViewGLSurfaceGLX : public GLSurfaceGLX {
 public:
  explicit UnmappedNativeViewGLSurfaceGLX(const gfx::Size& size);

  // Implement GLSurfaceGLX.
  bool Initialize(GLSurfaceFormat format) override;
  void Destroy() override;
  bool IsOffscreen() override;
  gfx::SwapResult SwapBuffers() override;
  gfx::Size GetSize() override;
  void* GetHandle() override;
  void* GetConfig() override;
  GLSurfaceFormat GetFormat() override;
  unsigned long GetCompatibilityKey() override;

 protected:
  ~UnmappedNativeViewGLSurfaceGLX() override;

 private:
  gfx::Size size_;
  GLXFBConfig config_ = nullptr;
  // Unmapped dummy window, used to provide a compatible surface.
  gfx::AcceleratedWidget window_ = 0;

  // GLXDrawable for the window.
  GLXWindow glx_window_ = 0;

  DISALLOW_COPY_AND_ASSIGN(UnmappedNativeViewGLSurfaceGLX);
};

}  // namespace gl

#endif  // UI_GL_GL_SURFACE_GLX_H_
