// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/vaapi_jpeg_encoder.h"

#include <stddef.h>
#include <string.h>

#include "base/logging.h"
#include "base/macros.h"
#include "media/filters/jpeg_parser.h"

namespace media {

// static
bool VaapiJpegEncoder::Encode(VaapiWrapper* vaapi_wrapper,
                     int width, int height,
                     VASurfaceID va_surface) {
  return false;
}

} // namespace media
