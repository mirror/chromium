// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/va_surface.h"

namespace media {

VASurface::VASurface(VASurfaceID va_surface_id,
                     const gfx::Size& size,
                     unsigned int format,
                     const ReleaseCB& release_cb)
    : va_surface_id_(va_surface_id),
      size_(size),
      format_(format),
      release_cb_(release_cb) {
  DCHECK(!release_cb_.is_null());
  DCHECK(format == VA_RT_FORMAT_YUV400 || format == VA_RT_FORMAT_YUV411 ||
         format == VA_RT_FORMAT_YUV420 || format == VA_RT_FORMAT_YUV422 ||
         format == VA_RT_FORMAT_YUV444)
      << "Only YUV VA_RT_FORMATs supported";
}

VASurface::~VASurface() {
  std::move(release_cb_).Run(va_surface_id_);
}

}  // namespace media
