// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/gpu_memory_buffer_factory_native_pixmap.h"

#include "ui/gfx/client_native_pixmap.h"
#include "ui/gfx/native_pixmap.h"
#include "ui/gl/gl_image_native_pixmap.h"

#if defined(USE_OZONE)
#include "ui/ozone/public/ozone_platform.h"
#include "ui/ozone/public/surface_factory_ozone.h"
#endif

#ifndef DRM_FORMAT_MOD_INVALID
#define DRM_FORMAT_MOD_INVALID ((1ULL << 56) - 1)
#endif

namespace gpu {

GpuMemoryBufferFactoryNativePixmap::GpuMemoryBufferFactoryNativePixmap() {}

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

  // If CreateGpuMemoryBuffer was used to allocate this buffer then avoid
  // creating a new native pixmap for it.
  {
    base::AutoLock lock(native_pixmaps_lock_);
    NativePixmapMapKey key(handle.id.id, client_id);
    auto it = native_pixmaps_.find(key);
    if (it != native_pixmaps_.end())
      pixmap = it->second;
  }

  // Create new pixmap from handle if one doesn't already exist.
  if (!pixmap) {
#if defined(USE_OZONE)
    pixmap = ui::OzonePlatform::GetInstance()
                 ->GetSurfaceFactoryOzone()
                 ->CreateNativePixmapFromHandle(surface_handle, size, format,
                                                handle.native_pixmap_handle);
#else
    // TODO(j.isorce): implement this to enable glCreateImageCHROMIUM on Linux.
    // On going in http://codereview.chromium.org/2705213005, crbug.com/584248.
    NOTIMPLEMENTED();
#endif
    if (!pixmap.get()) {
      DLOG(ERROR) << "Failed to create pixmap from handle";
      return nullptr;
    }
  } else {
    for (const auto& fd : handle.native_pixmap_handle.fds) {
      // Close the fd by wrapping it in a ScopedFD and letting it fall
      // out of scope.
      base::ScopedFD scoped_fd(fd.fd);
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

std::vector<gfx::BufferAttribute>
GpuMemoryBufferFactoryNativePixmap::GetBufferAttributesForImage() {
  if (!image_buffer_attributes_.empty())
    return image_buffer_attributes_;

  image_buffer_attributes_ =
      gl::GLImageNativePixmap::QueryDmaBufBufferAttributes();
  std::sort(image_buffer_attributes_.begin(), image_buffer_attributes_.end());
#if defined(USE_OZONE)
  std::vector<gfx::BufferAttribute> intersection;
  std::vector<gfx::BufferAttribute> drm_buffer_attributes =
      ui::OzonePlatform::GetInstance()
          ->GetSurfaceFactoryOzone()
          ->GetBufferAttributesForScanout(gfx::kNullAcceleratedWidget);
  std::sort(drm_buffer_attributes.begin(), drm_buffer_attributes.end());

  // NOTE(varad): If either of the buffer consumers we've queried (EGL or
  // SurfaceFactoryOzone) returns the DRM_FORMAT_MOD_INVALID modifier with a
  // common format, we advertise only the invalid modifier since we cannot
  // guarantee a modifier in that case.
  size_t i = 0, j = 0;
  while (i < image_buffer_attributes_.size() &&
         j < drm_buffer_attributes.size()) {
    if (image_buffer_attributes_[i].fourcc == drm_buffer_attributes[j].fourcc) {
      bool has_invalid =
          (image_buffer_attributes_[i].modifier == DRM_FORMAT_MOD_INVALID ||
           drm_buffer_attributes[j].modifier == DRM_FORMAT_MOD_INVALID);
      if (has_invalid || image_buffer_attributes_[i].modifier ==
                             drm_buffer_attributes[j].modifier) {
        intersection.push_back(gfx::BufferAttribute(
            image_buffer_attributes_[i].fourcc,
            has_invalid ? DRM_FORMAT_MOD_INVALID
                        : image_buffer_attributes_[i].modifier));
        i++;
        j++;
        continue;
      }
    }
    if (image_buffer_attributes_[i] < drm_buffer_attributes[j])
      i++;
    else
      j++;
  }

  image_buffer_attributes_ = std::move(intersection);
#endif
  return image_buffer_attributes_;
}

unsigned GpuMemoryBufferFactoryNativePixmap::RequiredTextureType() {
  return GL_TEXTURE_2D;
}

}  // namespace gpu
