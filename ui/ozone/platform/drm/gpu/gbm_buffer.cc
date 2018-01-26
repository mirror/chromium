// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/gpu/gbm_buffer.h"

#include <drm.h>
#include <drm_fourcc.h>
#include <fcntl.h>
#include <gbm.h>
#include <xf86drm.h>
#include <utility>

#include "base/files/platform_file.h"
#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/trace_event.h"
#include "ui/gfx/buffer_format_util.h"
#include "ui/gfx/geometry/size_conversions.h"
#include "ui/gfx/native_pixmap_handle.h"
#include "ui/ozone/platform/drm/common/drm_util.h"
#include "ui/ozone/platform/drm/gpu/drm_window.h"
#include "ui/ozone/platform/drm/gpu/gbm_device.h"
#include "ui/ozone/platform/drm/gpu/gbm_surface_factory.h"
#include "ui/ozone/platform/drm/gpu/gbm_surfaceless.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/ozone/public/surface_factory_ozone.h"

static_assert(gfx::NativePixmapPlane::kNoModifier == DRM_FORMAT_MOD_INVALID, "mismatch");

namespace ui {

GbmBuffer::GbmBuffer(const scoped_refptr<GbmDevice>& gbm,
                     gbm_bo* bo,
                     uint32_t flags,
                     uint64_t modifier)
    : drm_(gbm), bo_(bo) {
  if (flags & GBM_BO_USE_SCANOUT) {
    DCHECK(bo_);
    framebuffer_pixel_format_ = gbm_bo_get_format(bo);
    opaque_framebuffer_pixel_format_ = GetFourCCFormatForOpaqueFramebuffer(
        GetBufferFormatFromFourCCFormat(framebuffer_pixel_format_));
    format_modifier_ = modifier;

    uint32_t handles[4] = {0};
    uint32_t strides[4] = {0};
    uint32_t offsets[4] = {0};
    uint64_t modifiers[4] = {0};

    for (size_t i = 0; i < gbm_bo_get_num_planes(bo); ++i) {
      handles[i] = gbm_bo_get_plane_handle(bo, i).u32;
      strides[i] = gbm_bo_get_plane_stride(bo, i);
      offsets[i] = gbm_bo_get_plane_offset(bo, i);
      if (modifier != DRM_FORMAT_MOD_INVALID)
        modifiers[i] = modifier;
    }

    // AddFramebuffer2 only considers the modifiers if addfb_flags has
    // DRM_MODE_FB_MODIFIERS set. We only set that when we've created
    // a bo with modifiers, otherwise, we rely on the "no modifiers"
    // behavior doing the right thing.
    const uint32_t addfb_flags =
        modifier != DRM_FORMAT_MOD_INVALID ? DRM_MODE_FB_MODIFIERS : 0;
    bool ret = drm_->AddFramebuffer2(
        gbm_bo_get_width(bo), gbm_bo_get_height(bo), framebuffer_pixel_format_,
        handles, strides, offsets, modifiers, &framebuffer_, addfb_flags);
    // TODO(dcastagna): remove debugging info once we figure out why
    // AddFramebuffer2 is failing. crbug.com/789292
    if (!ret) {
      PLOG(ERROR) << "AddFramebuffer2 failed: ";
      LOG(ERROR) << base::StringPrintf(
          "planes: %zu, width: %u, height: %u, addfb_flags: %u",
          gbm_bo_get_num_planes(bo), gbm_bo_get_width(bo),
          gbm_bo_get_height(bo), addfb_flags);
      for (size_t i = 0; i < gbm_bo_get_num_planes(bo); ++i) {
        LOG(ERROR) << "handles: " << handles[i] << ", stride: " << strides[i]
                   << ", offset: " << offsets[i]
                   << ", modifier: " << modifiers[i];
      }
    }
    DCHECK(ret);
    if (opaque_framebuffer_pixel_format_ != framebuffer_pixel_format_) {
      ret = drm_->AddFramebuffer2(gbm_bo_get_width(bo), gbm_bo_get_height(bo),
                                  opaque_framebuffer_pixel_format_, handles,
                                  strides, offsets, modifiers,
                                  &opaque_framebuffer_, addfb_flags);
      PLOG_IF(ERROR, !ret) << "AddFramebuffer2 failed";
      DCHECK(ret);
    }
  }
}

GbmBuffer::~GbmBuffer() {
  if (framebuffer_)
    drm_->RemoveFramebuffer(framebuffer_);
  if (opaque_framebuffer_)
    drm_->RemoveFramebuffer(opaque_framebuffer_);
  if (bo())
    gbm_bo_destroy(bo());
}

uint32_t GbmBuffer::GetFormat() const {
  return gbm_bo_get_format(bo_);
}

size_t GbmBuffer::GetFdCount() const {
  return 1;
}

int GbmBuffer::GetFd(size_t index) const {
  DCHECK_LT(index, 1);
  return gbm_bo_get_fd(bo_);
}

int GbmBuffer::GetStride(size_t index) const {
  DCHECK_LT(index, gbm_bo_get_num_planes(bo_));
  return gbm_bo_get_plane_stride(bo_, index);
}

int GbmBuffer::GetOffset(size_t index) const {
  DCHECK_LT(index, gbm_bo_get_num_planes(bo_));
  return gbm_bo_get_plane_offset(bo_, index);
}

size_t GbmBuffer::GetSize(size_t index) const {
  DCHECK_LT(index, gbm_bo_get_num_planes(bo_));
  return gbm_bo_get_plane_size(bo_, index);
}

uint32_t GbmBuffer::GetFramebufferId() const {
  return framebuffer_;
}

uint32_t GbmBuffer::GetOpaqueFramebufferId() const {
  return opaque_framebuffer_ ? opaque_framebuffer_ : framebuffer_;
}

uint32_t GbmBuffer::GetHandle() const {
  return bo() ? gbm_bo_get_handle(bo()).u32 : 0;
}

gfx::Size GbmBuffer::GetSize() const {
  return gfx::Size(gbm_bo_get_width(bo_), gbm_bo_get_height(bo_));
}

uint32_t GbmBuffer::GetFramebufferPixelFormat() const {
  DCHECK(framebuffer_);
  return framebuffer_pixel_format_;
}

uint32_t GbmBuffer::GetOpaqueFramebufferPixelFormat() const {
  DCHECK(framebuffer_);
  return opaque_framebuffer_pixel_format_;
}

uint64_t GbmBuffer::GetFormatModifier() const {
  return format_modifier_;
}

const DrmDevice* GbmBuffer::GetDrmDevice() const {
  return drm_.get();
}

bool GbmBuffer::RequiresGlFinish() const {
  return !drm_->is_primary_device();
}

scoped_refptr<GbmBuffer> GbmBuffer::CreateBufferForBO(
    const scoped_refptr<GbmDevice>& gbm,
    gbm_bo* bo,
    uint32_t flags,
    uint64_t modifier) {
  scoped_refptr<GbmBuffer> buffer(
      new GbmBuffer(gbm, bo, flags, modifier));
  if (flags & GBM_BO_USE_SCANOUT && !buffer->GetFramebufferId())
    return nullptr;

  return buffer;
}

// static
scoped_refptr<GbmBuffer> GbmBuffer::CreateBufferWithModifiers(
    const scoped_refptr<GbmDevice>& gbm,
    uint32_t format,
    const gfx::Size& size,
    uint32_t flags,
    const std::vector<uint64_t>& modifiers) {
  TRACE_EVENT2("drm", "GbmBuffer::CreateBufferWithModifiers", "device",
               gbm->device_path().value(), "size", size.ToString());

  gbm_bo* bo =
      gbm_bo_create_with_modifiers(gbm->device(), size.width(), size.height(),
                                   format, modifiers.data(), modifiers.size());
  if (!bo)
    return nullptr;

  return CreateBufferForBO(gbm, bo, flags, gbm_bo_get_format_modifier(bo));
}

// static
scoped_refptr<GbmBuffer> GbmBuffer::CreateBuffer(
    const scoped_refptr<GbmDevice>& gbm,
    uint32_t format,
    const gfx::Size& size,
    uint32_t flags) {
  TRACE_EVENT2("drm", "GbmBuffer::CreateBuffer", "device",
               gbm->device_path().value(), "size", size.ToString());

  gbm_bo* bo =
      gbm_bo_create(gbm->device(), size.width(), size.height(), format, flags);
  if (!bo)
    return nullptr;

  // minigbm knows which modifier it chose for the bo it created, and will
  // tell us if we ask. However, we have to track it as DRM_FORMAT_MOD_INVALID
  // at this level so we don't try to pass it to addfb2, which may not support
  // that.  Only when the bo is created with modifiers
  // (CreateBufferWithModifiers above) do we track the actual modifier, since
  // we know we can pass it to addfb2 in that case.

  return CreateBufferForBO(gbm, bo, flags, DRM_FORMAT_MOD_INVALID);
}

// static
scoped_refptr<GbmBuffer> GbmBuffer::CreateBufferFromFds(
    const scoped_refptr<GbmDevice>& gbm,
    uint32_t format,
    const gfx::Size& size,
    std::vector<base::ScopedFD>&& fds,
    const std::vector<gfx::NativePixmapPlane>& planes) {
  TRACE_EVENT2("drm", "GbmBuffer::CreateBufferFromFD", "device",
               gbm->device_path().value(), "size", size.ToString());
  DCHECK_LE(fds.size(), planes.size());
  DCHECK_EQ(planes[0].offset, 0);

  // Try to use scanout if supported.
  int gbm_flags = GBM_BO_USE_SCANOUT | GBM_BO_USE_TEXTURING;
  if (!gbm_device_is_format_supported(gbm->device(), format, gbm_flags))
    gbm_flags &= ~GBM_BO_USE_SCANOUT;

  gbm_bo* bo = nullptr;
  if (gbm_device_is_format_supported(gbm->device(), format, gbm_flags)) {
    struct gbm_import_fd_planar_data fd_data;
    fd_data.width = size.width();
    fd_data.height = size.height();
    fd_data.format = format;

    DCHECK_LE(planes.size(), 3u);
    for (size_t i = 0; i < planes.size(); ++i) {
      fd_data.fds[i] = fds[i < fds.size() ? i : 0].get();
      fd_data.strides[i] = planes[i].stride;
      fd_data.offsets[i] = planes[i].offset;
      fd_data.format_modifiers[i] = planes[i].modifier;
    }

    bo = gbm_bo_import(gbm->device(), GBM_BO_IMPORT_FD_PLANAR, &fd_data,
                       gbm_flags);
    if (!bo) {
      LOG(ERROR) << "nullptr returned from gbm_bo_import";
      return nullptr;
    }

//    for (size_t i = 0; i < fds.size(); ++i)
//      close(fds[i]);
  }

  scoped_refptr<GbmBuffer> buffer(new GbmBuffer(gbm, bo, gbm_flags,
                                                planes[0].modifier));

  return buffer;
}

GbmPixmap::GbmPixmap(GbmSurfaceFactory* surface_manager,
                     const scoped_refptr<GbmBuffer>& buffer)
    : surface_manager_(surface_manager), buffer_(buffer) {}

gfx::NativePixmapHandle GbmPixmap::ExportHandle() {
  gfx::NativePixmapHandle handle;
  gfx::BufferFormat format =
      ui::GetBufferFormatFromFourCCFormat(buffer_->GetFormat());
  // TODO(dcastagna): Use gbm_bo_get_num_planes once all the formats we use are
  // supported by gbm.
  for (size_t i = 0; i < gfx::NumberOfPlanesForBufferFormat(format); ++i) {
    handle.planes.emplace_back(buffer_->GetStride(i), buffer_->GetOffset(i),
                               buffer_->GetSize(i),
                               buffer_->GetFormatModifier());
  }

  // No GBM bo have more than one fd.
  base::ScopedFD scoped_fd(buffer_->GetFd(0));
  if (!scoped_fd.is_valid()) {
    PLOG(ERROR) << "GetFd";
    return gfx::NativePixmapHandle();
  }
  handle.fds.emplace_back(
    base::FileDescriptor(scoped_fd.release(), true /* auto_close */));

  return handle;
}

GbmPixmap::~GbmPixmap() {
}

void* GbmPixmap::GetEGLClientBuffer() const {
  return nullptr;
}

bool GbmPixmap::AreDmaBufFdsValid() const {
  return true;
}

size_t GbmPixmap::GetDmaBufFdCount() const {
  return buffer_->GetFdCount();
}

int GbmPixmap::GetDmaBufFd(size_t plane) const {
  return buffer_->GetFd(plane);
}

int GbmPixmap::GetDmaBufPitch(size_t plane) const {
  return buffer_->GetStride(plane);
}

int GbmPixmap::GetDmaBufOffset(size_t plane) const {
  return buffer_->GetOffset(plane);
}

uint64_t GbmPixmap::GetDmaBufModifier(size_t plane) const {
  return buffer_->GetFormatModifier();
}

gfx::BufferFormat GbmPixmap::GetBufferFormat() const {
  return ui::GetBufferFormatFromFourCCFormat(buffer_->GetFormat());
}

gfx::Size GbmPixmap::GetBufferSize() const {
  return buffer_->GetSize();
}

uint32_t GbmPixmap::GetUniqueId() const {
  return buffer_->GetHandle();
}

bool GbmPixmap::ScheduleOverlayPlane(gfx::AcceleratedWidget widget,
                                     int plane_z_order,
                                     gfx::OverlayTransform plane_transform,
                                     const gfx::Rect& display_bounds,
                                     const gfx::RectF& crop_rect) {
  DCHECK_NE(buffer_->GetFramebufferId(), 0);
  surface_manager_->GetSurface(widget)->QueueOverlayPlane(
      OverlayPlane(buffer_, plane_z_order, plane_transform, display_bounds,
                   crop_rect, base::kInvalidPlatformFile));

  return true;
}

}  // namespace ui
