// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/arc/protected_buffer_allocator.h"

#include "base/files/scoped_file.h"
#include "media/gpu/format_utils.h"
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
    ProtectedBufferManager* protected_buffer_manager)
    : protected_buffer_manager_(protected_buffer_manager) {}

ProtectedBufferAllocator::~ProtectedBufferAllocator() {
  protected_buffer_manager_->DeallocateAllProtectedBuffers();
}

void ProtectedBufferAllocator::AllocateProtectedBuffer(
    mojom::BufferType type,
    mojom::ScopedHandle handle_fd,
    mojom::BufferMetadata metadata,
    AllocateProtectedBufferCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  bool result = false;
  switch (type) {
    case mojom::BufferType::SHARED_MEMORY:
      base::ScopedFD fd = UnwrapFdFromMojoHandle(std::move(handle_fd));
      if (!fd.is_valid()) {
        std::move(callback).Run(false);
        return;
      }
      result = protected_buffer_manager_->AllocateProtectedSharedMemory(
          std::move(fd), metadata->size);
      break;
    case mojom::BufferType::NATIVE_PIXMAP:
      base::ScopedFD fd = UnwrapFdFromMojoHandle(std::move(handle_fd));
      if (!fd.is_valid()) {
        std::move(callback).Run(false);
        return;
      }
      gfx::Size size = metatda->picture_size;
      result = protected_buffer_manager_->AllocateProtectedNativePixmap(
          std::move(fd),
          media::VideoPixelFormatToGfxBufferFormat(metadata->pixel_format),
          size);
      break;
    default:
      VLOGF(1) << "Unknown buffer type: " << static_cast<int32_t>(type);
      return;
  }
  std::move(callback).Run(result);
}

void ProtectedBufferAllocator::DeallocateProtectedBuffer(
    mojom::ScopedHandle handle_fd,
    DeallocateProtectedBufferCallback callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  base::ScopedFD fd = UnwrapFdFromMojoHandle(std::move(handle_fd));
  if (!fd.is_valid()) {
    std::move(callback).Run(false);
    return;
  }
  std::move(callback).Run(
      protected_buffer_manager_->DeallocateProtectedBuffer(std::move(fd)));
}
}  // namespace arc
}
