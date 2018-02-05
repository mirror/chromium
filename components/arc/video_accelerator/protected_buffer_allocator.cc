// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/video_accelerator/protected_buffer_allocator.h"

#include <utility>

#include "base/files/scoped_file.h"
#include "components/arc/video_accelerator/protected_buffer_manager.h"
#include "media/gpu/format_utils.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "ui/gfx/geometry/size.h"

#define VLOGF(level) VLOG(level) << __func__ << "(): "

namespace {

base::ScopedFD UnwrapFdFromMojoHandle(mojo::ScopedHandle handle) {
  if (!handle.is_valid()) {
    VLOGF(1) << "Handle is invalid.";
    return base::ScopedFD();
  }

  base::PlatformFile platform_file;
  MojoResult mojo_result =
      mojo::UnwrapPlatformFile(std::move(handle), &platform_file);
  if (mojo_result != MOJO_RESULT_OK) {
    VLOGF(1) << "UnwrapPlatformFile failed: " << mojo_result;
    return base::ScopedFD();
  }
  return base::ScopedFD(platform_file);
}
}  // namespace

namespace arc {
ProtectedBufferAllocator::ProtectedBufferAllocator(
    ProtectedBufferManager* protected_buffer_manager,
    base::OnceClosure cb_)
    : protected_buffer_manager_(protected_buffer_manager),
      reset_cb_(std::move(cb_)) {
  VLOGF(2);
}

ProtectedBufferAllocator::~ProtectedBufferAllocator() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  VLOGF(2);
  protected_buffer_manager_->DeallocateAllProtectedBuffers();
  if (!reset_cb_) {
    std::move(reset_cb_).Run();
  }
}

void ProtectedBufferAllocator::AllocateProtectedBuffer(
    mojom::ProtectedBufferType type,
    mojo::ScopedHandle handle_fd,
    mojom::ProtectedBufferMetadataPtr metadata,
    AllocateProtectedBufferCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  VLOGF(2) << "type=" << static_cast<uint32_t>(type);
  bool result = false;
  switch (type) {
    case mojom::ProtectedBufferType::SHARED_MEMORY: {
      base::ScopedFD fd = UnwrapFdFromMojoHandle(std::move(handle_fd));
      if (!fd.is_valid()) {
        std::move(callback).Run(false);
        return;
      }
      VLOGF(2) << "size=" << metadata->size;
      result = protected_buffer_manager_->AllocateProtectedSharedMemory(
          std::move(fd), metadata->size);
      break;
    }
    case mojom::ProtectedBufferType::NATIVE_PIXMAP: {
      base::ScopedFD fd = UnwrapFdFromMojoHandle(std::move(handle_fd));
      if (!fd.is_valid()) {
        std::move(callback).Run(false);
        return;
      }
      media::VideoPixelFormat pixel_format;
      switch (metadata->pixel_format) {
        case arc::mojom::HalPixelFormat::HAL_PIXEL_FORMAT_YV12:
          pixel_format = media::PIXEL_FORMAT_YV12;
          break;
        case arc::mojom::HalPixelFormat::HAL_PIXEL_FORMAT_NV12:
          pixel_format = media::PIXEL_FORMAT_NV12;
          break;
        default:
          VLOGF(1) << "Unsupported format: " << metadata->pixel_format;
          std::move(callback).Run(false);
          return;
      }
      VLOGF(2) << "format=" << static_cast<uint32_t>(metadata->pixel_format)
               << ", width=" << metadata->picture_size.width()
               << ", height=" << metadata->picture_size.height();
      result = protected_buffer_manager_->AllocateProtectedNativePixmap(
          std::move(fd), media::VideoPixelFormatToGfxBufferFormat(pixel_format),
          gfx::Size(metadata->picture_size));
      break;
    }
    default:
      VLOGF(1) << "Unknown buffer type: " << static_cast<int32_t>(type);
      return;
  }
  std::move(callback).Run(result);
}

void ProtectedBufferAllocator::DeallocateProtectedBuffer(
    mojo::ScopedHandle handle_fd,
    DeallocateProtectedBufferCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  VLOGF(2);
  base::ScopedFD fd = UnwrapFdFromMojoHandle(std::move(handle_fd));
  if (!fd.is_valid()) {
    std::move(callback).Run(false);
    return;
  }
  std::move(callback).Run(
      protected_buffer_manager_->DeallocateProtectedBuffer(std::move(fd)));
}
}  // namespace arc
