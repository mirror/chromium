// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/headless/headless_surface_factory.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/task_scheduler/post_task.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/native_pixmap.h"
#include "ui/gfx/skia_util.h"
#include "ui/gfx/vsync_provider.h"
#include "ui/gl/gl_features.h"
#include "ui/gl/gl_surface_egl.h"
#include "ui/ozone/common/egl_util.h"
#include "ui/ozone/common/gl_ozone_egl.h"
#include "ui/ozone/common/gl_ozone_osmesa.h"
#include "ui/ozone/platform/headless/headless_window.h"
#include "ui/ozone/platform/headless/headless_window_manager.h"
#include "ui/ozone/public/surface_ozone_canvas.h"

namespace ui {

namespace {

void WriteDataToFile(const base::FilePath& location, const SkBitmap& bitmap) {
  DCHECK(!location.empty());
  std::vector<unsigned char> png_data;
  gfx::PNGCodec::FastEncodeBGRASkBitmap(bitmap, true, &png_data);
  base::WriteFile(location, reinterpret_cast<const char*>(png_data.data()),
                  png_data.size());
}

// TODO(altimin): Find a proper way to capture rendering output.
class FileSurface : public SurfaceOzoneCanvas {
 public:
  explicit FileSurface(const base::FilePath& location) : location_(location) {}
  ~FileSurface() override {}

  // SurfaceOzoneCanvas overrides:
  void ResizeCanvas(const gfx::Size& viewport_size) override {
    surface_ = SkSurface::MakeRaster(SkImageInfo::MakeN32Premul(
        viewport_size.width(), viewport_size.height()));
  }
  sk_sp<SkSurface> GetSurface() override { return surface_; }
  void PresentCanvas(const gfx::Rect& damage) override {
    if (location_.empty())
      return;
    SkBitmap bitmap;
    bitmap.allocPixels(surface_->getCanvas()->imageInfo());

    // TODO(dnicoara) Use SkImage instead to potentially avoid a copy.
    // See crbug.com/361605 for details.
    if (surface_->getCanvas()->readPixels(bitmap, 0, 0)) {
      base::PostTaskWithTraits(
          FROM_HERE,
          {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
          base::Bind(&WriteDataToFile, location_, bitmap));
    }
  }
  std::unique_ptr<gfx::VSyncProvider> CreateVSyncProvider() override {
    return nullptr;
  }

 private:
  base::FilePath location_;
  sk_sp<SkSurface> surface_;
};

class TestPixmap : public gfx::NativePixmap {
 public:
  explicit TestPixmap(gfx::BufferFormat format) : format_(format) {}

  void* GetEGLClientBuffer() const override { return nullptr; }
  bool AreDmaBufFdsValid() const override { return false; }
  size_t GetDmaBufFdCount() const override { return 0; }
  int GetDmaBufFd(size_t plane) const override { return -1; }
  int GetDmaBufPitch(size_t plane) const override { return 0; }
  int GetDmaBufOffset(size_t plane) const override { return 0; }
  uint64_t GetDmaBufModifier(size_t plane) const override { return 0; }
  gfx::BufferFormat GetBufferFormat() const override { return format_; }
  gfx::Size GetBufferSize() const override { return gfx::Size(); }
  bool ScheduleOverlayPlane(gfx::AcceleratedWidget widget,
                            int plane_z_order,
                            gfx::OverlayTransform plane_transform,
                            const gfx::Rect& display_bounds,
                            const gfx::RectF& crop_rect) override {
    return true;
  }
  void SetProcessingCallback(
      const ProcessingCallback& processing_callback) override {}
  gfx::NativePixmapHandle ExportHandle() override {
    return gfx::NativePixmapHandle();
  }

 private:
  ~TestPixmap() override {}

  gfx::BufferFormat format_;

  DISALLOW_COPY_AND_ASSIGN(TestPixmap);
};

// A thin wrapper around PbufferGLSurfaceEGL that pretends it is an onscreen
// surface.
class GL_EXPORT GLSurfaceEGLHeadless : public gl::PbufferGLSurfaceEGL {
 public:
  GLSurfaceEGLHeadless() : PbufferGLSurfaceEGL(gfx::Size(1, 1)) {}

  bool IsOffscreen() override { return false; }
  gfx::SwapResult SwapBuffers() override { return gfx::SwapResult::SWAP_ACK; }

 protected:
  ~GLSurfaceEGLHeadless() override { Destroy(); }

 private:
  DISALLOW_COPY_AND_ASSIGN(GLSurfaceEGLHeadless);
};

// Headless EGL implementation for use with SwiftShader.
class GLOzoneEGLHeadless : public GLOzoneEGL {
 public:
  GLOzoneEGLHeadless() = default;
  ~GLOzoneEGLHeadless() override = default;

  // GLOzone:
  scoped_refptr<gl::GLSurface> CreateViewGLSurface(
      gfx::AcceleratedWidget window) override {
    return gl::InitializeGLSurface(new GLSurfaceEGLHeadless());
  }

  scoped_refptr<gl::GLSurface> CreateOffscreenGLSurface(
      const gfx::Size& size) override {
    return gl::InitializeGLSurface(new gl::PbufferGLSurfaceEGL(size));
  }

 protected:
  // GLOzoneEGL:
  intptr_t GetNativeDisplay() override { return EGL_DEFAULT_DISPLAY; }

  bool LoadGLES2Bindings(gl::GLImplementation implementation) override {
    return LoadDefaultEGLGLES2Bindings(implementation);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(GLOzoneEGLHeadless);
};

}  // namespace

HeadlessSurfaceFactory::HeadlessSurfaceFactory(
    HeadlessWindowManager* window_manager)
    : window_manager_(window_manager),
      osmesa_implementation_(base::MakeUnique<GLOzoneOSMesa>()),
      swiftshader_implementation_(base::MakeUnique<GLOzoneEGLHeadless>()) {}

HeadlessSurfaceFactory::~HeadlessSurfaceFactory() {}

std::vector<gl::GLImplementation>
HeadlessSurfaceFactory::GetAllowedGLImplementations() {
  std::vector<gl::GLImplementation> impls;
#if BUILDFLAG(ENABLE_SWIFTSHADER)
  impls.push_back(gl::kGLImplementationSwiftShaderGL);
#endif
  impls.push_back(gl::kGLImplementationOSMesaGL);
  return impls;
}

GLOzone* HeadlessSurfaceFactory::GetGLOzone(
    gl::GLImplementation implementation) {
  switch (implementation) {
    case gl::kGLImplementationOSMesaGL:
      return osmesa_implementation_.get();
    case gl::kGLImplementationSwiftShaderGL:
    case gl::kGLImplementationEGLGLES2:
      return swiftshader_implementation_.get();
    default:
      return nullptr;
  }
}

std::unique_ptr<SurfaceOzoneCanvas>
HeadlessSurfaceFactory::CreateCanvasForWidget(gfx::AcceleratedWidget widget) {
  HeadlessWindow* window = window_manager_->GetWindow(widget);
  return base::MakeUnique<FileSurface>(window->path());
}

scoped_refptr<gfx::NativePixmap> HeadlessSurfaceFactory::CreateNativePixmap(
    gfx::AcceleratedWidget widget,
    gfx::Size size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage) {
  return new TestPixmap(format);
}

}  // namespace ui
