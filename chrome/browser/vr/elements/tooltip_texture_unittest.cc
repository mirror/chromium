// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/tooltip_texture.h"

#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/canvas.h"

namespace vr {

TEST(TooltipTextureTest, Visualize) {
  TooltipTexture texture;
  texture.SetSize(0.58f, 0.1f);
  texture.SetText(base::ASCIIToUTF16(
      "test test test test test test test test test test test test test test "
      "test test test test test test test test"));
  texture.SetBackgroundColor(0xE0EFEFEF);
  texture.SetTextColor(0xFF212121);

  auto size = texture.GetPreferredTextureSize(512);

  SkBitmap bitmap;
  CHECK(bitmap.tryAllocN32Pixels(size.width(), size.height(), false));
  SkCanvas canvas(bitmap);
  canvas.drawColor(0x00000000);

  texture.DrawAndLayout(&canvas, size);

  SkFILEWStream stream("texture.png");
  SkEncodeImage(&stream, bitmap, SkEncodedImageFormat::kPNG, 100);
}

}  // namespace vr
