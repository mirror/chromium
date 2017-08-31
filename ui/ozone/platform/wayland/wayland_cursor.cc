// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/wayland_cursor.h"

#include <sys/mman.h>
#include <vector>

#include "base/logging.h"
#include "base/memory/shared_memory.h"
#include "ui/ozone/platform/wayland/wayland_connection.h"
#include "ui/ozone/platform/wayland/wayland_pointer.h"

namespace ui {

WaylandCursor::WaylandCursor() : sh_memory_(new base::SharedMemory()) {}

void WaylandCursor::Init(wl_pointer* pointer, WaylandConnection* connection) {
  if (input_pointer_ == pointer)
    return;

  input_pointer_ = pointer;

  DCHECK(connection);
  shm_ = connection->shm();
  pointer_surface_.reset(
      wl_compositor_create_surface(connection->compositor()));
}

WaylandCursor::~WaylandCursor() {
  pointer_surface_.reset();
  buffer_.reset();

  if (sh_memory_->handle().GetHandle()) {
    sh_memory_->Unmap();
    sh_memory_->Close();
  }
}

void WaylandCursor::UpdateBitmap(const std::vector<SkBitmap>& cursor_image,
                                 const gfx::Point& location,
                                 uint32_t serial) {
  if (!input_pointer_)
    return;

  if (!cursor_image.size()) {
    HideCursor(serial);
    return;
  }

  const SkBitmap& image = cursor_image[0];
  int width = image.width();
  int height = image.height();
  if (!width || !height) {
    HideCursor(serial);
    return;
  }

  if (!CreateSHMBuffer(width, height)) {
    LOG(ERROR) << "Failed to create SHM buffer for Cursor Bitmap.";
    wl_pointer_set_cursor(input_pointer_, serial, nullptr, 0, 0);
    return;
  }

  // The |bitmap| contains ARGB image, so just copy it.
  memcpy(sh_memory_->memory(), image.getPixels(), width_ * height_ * 4);

  wl_pointer_set_cursor(input_pointer_, serial, pointer_surface_.get(),
                        location.x(), location.y());
  wl_surface_attach(pointer_surface_.get(), buffer_.get(), 0, 0);
  wl_surface_damage(pointer_surface_.get(), 0, 0, width_, height_);
  wl_surface_commit(pointer_surface_.get());
}

bool WaylandCursor::CreateSHMBuffer(int width, int height) {
  if (width == width_ && height == height_)
    return true;

  struct wl_shm_pool* pool;
  int size, stride;

  width_ = width;
  height_ = height;
  stride = width_ * 4;
  SkImageInfo info = SkImageInfo::MakeN32Premul(width_, height_);
  size = info.getSafeSize(stride);

  if (sh_memory_->handle().GetHandle()) {
    sh_memory_->Unmap();
    sh_memory_->Close();
  }

  if (!sh_memory_->CreateAndMapAnonymous(size)) {
    LOG(ERROR) << "Create and mmap failed.";
    return false;
  }

  pool = wl_shm_create_pool(shm_, sh_memory_->handle().GetHandle(), size);
  buffer_.reset(wl_shm_pool_create_buffer(pool, 0, width_, height_, stride,
                                          WL_SHM_FORMAT_ARGB8888));
  wl_shm_pool_destroy(pool);
  return true;
}

void WaylandCursor::HideCursor(uint32_t serial) {
  width_ = 0;
  height_ = 0;
  wl_pointer_set_cursor(input_pointer_, serial, nullptr, 0, 0);

  buffer_.reset();

  if (sh_memory_->handle().GetHandle()) {
    sh_memory_->Unmap();
    sh_memory_->Close();
  }
}

}  // namespace ui
