// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VR_TEST_PIXELTEST_UTILS_H_
#define CHROME_BROWSER_VR_TEST_PIXELTEST_UTILS_H_

#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/geometry/size.h"

namespace vr {

std::unique_ptr<SkBitmap> SaveCurrentFrameBufferToSkBitmap(
    const gfx::Size& size);
bool SaveSkBitmapToPng(const SkBitmap& bitmap, const std::string& filename);

}  // namespace vr

#endif  // CHROME_BROWSER_VR_TEST_PIXELTEST_UTILS_H_
