// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/gpu_memory_buffer_factory_native_pixmap.h"

#include "gpu/ipc/service/protected_gpu_memory_buffer_manager.h"
#include "ui/gfx/client_native_pixmap.h"
#include "ui/gfx/native_pixmap.h"
#include "ui/gl/gl_image_native_pixmap.h"

#if defined(USE_OZONE)
#include "ui/ozone/public/ozone_platform.h"
#include "ui/ozone/public/surface_factory_ozone.h"
#endif

namespace gpu {

GpuMemoryBufferFactoryNativePixmap::GpuMemoryBufferFactoryNativePixmap(
    ProtectedGpuMemoryBufferManager* protected_buffer_manager)
    : protected_buffer_manager_(protected_buffer_manager) {}

GpuMemoryBufferFactoryNativePixmap::~GpuMemoryBufferFactoryNativePixmap() {}

gfx::GpuMemoryBufferHandle
GpuMemoryBufferFactoryNativePixmap::CreateGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    int client_id,
    SurfaceHandle surface_handle) {
#if defined(USE_OZONE)
  scoped_refptr<gfx::NativePixmap> pixmap =
      ui::OzonePlatform::GetInstance()
          ->GetSurfaceFactoryOzone()
          ->CreateNativePixmap(surface_handle, size, format, usage);
  if (!pixmap.get()) {
    DLOG(ERROR) << "Failed to create pixmap " << size.ToString() << " format "
                << static_cast<int>(format) << ", usage "
                << static_cast<int>(usage);
    return gfx::GpuMemoryBufferHandle();
  }

  gfx::GpuMemoryBufferHandle new_handle;
  new_handle.type = gfx::NATIVE_PIXMAP;
  new_handle.id = id;
  new_handle.native_pixmap_handle = pixmap->ExportHandle();

  // TODO(reveman): Remove this once crbug.com/628334 has been fixed.
  {
    base::AutoLock lock(native_pixmaps_lock_);
    NativePixmapMapKey key(id.id, client_id);
    DCHECK(native_pixmaps_.find(key) == native_pixmaps_.end());
    native_pixmaps_[key] = pixmap;
  }

  return new_handle;
#else
  NOTIMPLEMENTED();
  return gfx::GpuMemoryBufferHandle();
#endif
}

void GpuMemoryBufferFactoryNativePixmap::DestroyGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    int client_id) {
  base::AutoLock lock(native_pixmaps_lock_);
  NativePixmapMapKey key(id.id, client_id);
  native_pixmaps_.erase(key);
}

ImageFactory* GpuMemoryBufferFactoryNativePixmap::AsImageFactory() {
  return this;
}

scoped_refptr<gl::GLImage>
GpuMemoryBufferFactoryNativePixmap::CreateImageForGpuMemoryBuffer(
    const gfx::GpuMemoryBufferHandle& handle,
    const gfx::Size& size,
    gfx::BufferFormat format,
    unsigned internalformat,
    int client_id,
    SurfaceHandle surface_handle) {
  DCHECK_EQ(handle.type, gfx::NATIVE_PIXMAP);

  scoped_refptr<gfx::NativePixmap> pixmap;
  gfx::GpuMemoryBufferHandle handle_to_import = handle;

  // If CreateGpuMemoryBuffer was used to allocate this buffer then avoid
  // creating a new native pixmap for it.
  {
    base::AutoLock lock(native_pixmaps_lock_);
    NativePixmapMapKey key(handle_to_import.id.id, client_id);
    auto it = native_pixmaps_.find(key);
    if (it != native_pixmaps_.end()) {
      pixmap = it->second;
      gfx::CloseGpuMemoryBufferHandle(&handle_to_import);
    }
  }

  if (!pixmap) {
#if defined(USE_OZONE)
    // No existing pixmap found, need to create a new one.
    // First, see if we have a protected buffer backing this handle. If so,
    // import that buffer's handle instead. In that case we keep a reference
    // to the backing protected buffer only, and never use the passed buffer.
    if (protected_buffer_manager_) {
      auto protected_buffer_handle =
          protected_buffer_manager_->GetProtectedGpuMemoryBufferFor(
              handle_to_import);
      if (!protected_buffer_handle.is_null()) {
        // If a protected buffer is found, it already has a reference
        // to the original handle, so we can close it now.
        gfx::CloseGpuMemoryBufferHandle(&handle_to_import);
        handle_to_import = protected_buffer_handle;
      }
    }

    // Import either the protected buffer's handle, if we found one, or the
    // original handle we got from the client otherwise.
    pixmap = ui::OzonePlatform::GetInstance()
                 ->GetSurfaceFactoryOzone()
                 ->CreateNativePixmapFromHandle(
                     surface_handle, size, format,
                     handle_to_import.native_pixmap_handle);
#else
    // TODO(j.isorce): implement this to enable glCreateImageCHROMIUM on Linux.
    // On going in http://codereview.chromium.org/2705213005, crbug.com/584248.
    NOTIMPLEMENTED();
#endif
    if (!pixmap) {
      // CreateNativePixmapFromHandle() takes ownership of handle and is
      // responsible for closing it even on failure, so we don't need to.
      DLOG(ERROR) << "Failed to create pixmap from handle";
      return nullptr;
    }
  }

  scoped_refptr<gl::GLImageNativePixmap> image(
      new gl::GLImageNativePixmap(size, internalformat));
  if (!image->Initialize(pixmap.get(), format)) {
    LOG(ERROR) << "Failed to create GLImage " << size.ToString() << " format "
               << static_cast<int>(format);
    return nullptr;
  }
  return image;
}

scoped_refptr<gl::GLImage>
GpuMemoryBufferFactoryNativePixmap::CreateAnonymousImage(
    const gfx::Size& size,
    gfx::BufferFormat format,
    unsigned internalformat) {
  scoped_refptr<gfx::NativePixmap> pixmap;
#if defined(USE_OZONE)
  pixmap = ui::OzonePlatform::GetInstance()
               ->GetSurfaceFactoryOzone()
               ->CreateNativePixmap(gpu::kNullSurfaceHandle, size, format,
                                    gfx::BufferUsage::SCANOUT);
#else
  NOTIMPLEMENTED();
#endif
  if (!pixmap.get()) {
    LOG(ERROR) << "Failed to create pixmap " << size.ToString() << " format "
               << static_cast<int>(format);
    return nullptr;
  }
  scoped_refptr<gl::GLImageNativePixmap> image(
      new gl::GLImageNativePixmap(size, internalformat));
  if (!image->Initialize(pixmap.get(), format)) {
    LOG(ERROR) << "Failed to create GLImage " << size.ToString() << " format "
               << static_cast<int>(format);
    return nullptr;
  }
  return image;
}

unsigned GpuMemoryBufferFactoryNativePixmap::RequiredTextureType() {
  return GL_TEXTURE_2D;
}

}  // namespace gpu
