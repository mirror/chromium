// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/test/pixeltest_utils.h"

#include "base/memory/ptr_util.h"
#include "third_party/skia/include/core/SkImageEncoder.h"
#include "third_party/skia/include/core/SkStream.h"
#include "ui/gl/gl_bindings.h"

namespace vr {

std::unique_ptr<SkBitmap> SaveCurrentFrameBufferToSkBitmap(
    const gfx::Size& size) {
  // Read pixels from frame buffer.
  uint32_t* pixels = new uint32_t[size.width() * size.height()];
  glReadPixels(0, 0, size.width(), size.height(), GL_RGBA, GL_UNSIGNED_BYTE,
               pixels);
  if (glGetError() != GL_NO_ERROR) {
    return nullptr;
  }

  // Flip image vertically since SkBitmap expects the pixels in a different
  // order.
  for (int stride = 0; stride < size.height() / 2; stride++) {
    for (int strideOffset = 0; strideOffset < size.width(); strideOffset++) {
      size_t topPixelIndex = size.width() * stride + strideOffset;
      size_t bottomPixelIndex =
          (size.height() - stride - 1) * size.width() + strideOffset;
      pixels[topPixelIndex] ^= pixels[bottomPixelIndex];
      pixels[bottomPixelIndex] ^= pixels[topPixelIndex];
      pixels[topPixelIndex] ^= pixels[bottomPixelIndex];
    }
  }

  // Create bitmap with pixels.
  std::unique_ptr<SkBitmap> bitmap = base::MakeUnique<SkBitmap>();
  if (!bitmap->installPixels(SkImageInfo::MakeN32(size.width(), size.height(),
                                                  kOpaque_SkAlphaType),
                             pixels, sizeof(uint32_t) * size.width(),
                             [](void* pixels, void* context) {
                               delete[] static_cast<uint32_t*>(pixels);
                             },
                             nullptr)) {
    return nullptr;
  }

  return bitmap;
}

bool SaveSkBitmapToPng(const SkBitmap& bitmap, const std::string& filename) {
  SkFILEWStream stream(filename.c_str());
  if (!stream.isValid()) {
    return false;
  }
  if (!SkEncodeImage(&stream, bitmap, SkEncodedImageFormat::kPNG, 100)) {
    return false;
  }
  return true;
}

}  // namespace vr
