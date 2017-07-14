// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/tiles/image_decode_cache.h"

#include "base/metrics/histogram_macros.h"

namespace cc {

void ImageDecodeCache::RecordImageMipLevelUMA(int mip_level) {
  UMA_HISTOGRAM_CUSTOM_COUNTS("Renderer4.ImageDecodeMipLevel", mip_level + 1, 1,
                              32, 32);
}

}  // namespace cc
