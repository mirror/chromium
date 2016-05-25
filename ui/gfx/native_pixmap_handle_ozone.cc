// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/native_pixmap_handle_ozone.h"

namespace gfx {

GbmBufferPlane::GbmBufferPlane() : stride(0), offset(0), modifier(0) {}

GbmBufferPlane::GbmBufferPlane(int stride, int offset, uint64_t modifier)
    : stride(stride), offset(offset), modifier(modifier) {}

GbmBufferPlane::GbmBufferPlane(const GbmBufferPlane& other) = default;

GbmBufferPlane::~GbmBufferPlane() {}

NativePixmapHandle::NativePixmapHandle() {}
NativePixmapHandle::NativePixmapHandle(const NativePixmapHandle& other) =
    default;

NativePixmapHandle::~NativePixmapHandle() {}

}  // namespace gfx
