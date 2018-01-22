// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/skia_helper.h"
#include "base/logging.h"

namespace viz {
void PopulateSkBitmapWithResource(SkBitmap* sk_bitmap,
                                  void* pixels,
                                  const gfx::Size& size,
                                  ResourceFormat format) {
  DCHECK_EQ(RGBA_8888, format);
  SkImageInfo info = SkImageInfo::MakeN32Premul(size.width(), size.height());
  sk_bitmap->installPixels(info, pixels, info.minRowBytes());
}

}  // namespace viz
