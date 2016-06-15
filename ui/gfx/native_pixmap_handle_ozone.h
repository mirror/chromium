// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_NATIVE_PIXMAP_HANDLE_OZONE_H_
#define UI_GFX_NATIVE_PIXMAP_HANDLE_OZONE_H_

#include <stdint.h>
#include <vector>

#include "base/file_descriptor_posix.h"
#include "ui/gfx/gfx_export.h"

namespace gfx {

struct GFX_EXPORT GbmBufferPlane {
  GbmBufferPlane();
  GbmBufferPlane(int stride, int offset, uint64_t modifier);
  GbmBufferPlane(const GbmBufferPlane& other);
  ~GbmBufferPlane();

  // The strides and offsets in bytes to be used when accessing the buffers via
  // a memory mapping. One per plane per entry.
  int stride;
  int offset;
  uint64_t modifier;
};

struct GFX_EXPORT NativePixmapHandle {
  NativePixmapHandle();
  NativePixmapHandle(const NativePixmapHandle& other);

  ~NativePixmapHandle();
  // File descriptors for the underlying memory objects (usually dmabufs).
  std::vector<base::FileDescriptor> fds;

  std::vector<GbmBufferPlane> planes;
};

}  // namespace gfx

#endif  // UI_GFX_NATIVE_PIXMAP_HANDLE_OZONE_H_
