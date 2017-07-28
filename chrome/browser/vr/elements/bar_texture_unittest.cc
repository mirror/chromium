// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/bar_texture.h"

#include "base/memory/ptr_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/canvas.h"

namespace vr {

TEST(BarTextureTest, Visualize) {
  BarTexture texture;
  texture.SetSize(0.1f, 0.01f);
  texture.SetColor(ColorScheme::GetColorScheme(ColorScheme::Mode::kModeNormal)
                       .location_bar_background);
  auto size = texture.GetPreferredTextureSize(1024);

  SkBitmap bitmap;
  CHECK(bitmap.tryAllocN32Pixels(size.width(), size.height(), false));
  SkCanvas canvas(bitmap);
  canvas.drawColor(0x00000000);

  texture.DrawAndLayout(&canvas, size);

  SkFILEWStream stream("texture.png");
  SkEncodeImage(&stream, bitmap, SkEncodedImageFormat::kPNG, 100);
}

}  // namespace vr
