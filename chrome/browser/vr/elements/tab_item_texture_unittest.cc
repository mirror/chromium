// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/tab_item_texture.h"

#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/grit/vr_shell_resources.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"
#include "ui/gfx/canvas.h"

namespace vr {

TEST(TabItemTextureTest, Visualize) {
  // base::FilePath pak_path;
  // PathService::Get(base::DIR_MODULE, &pak_path);
  // ui::ResourceBundle::InitSharedInstanceWithPakPath(
  //       pak_path.AppendASCII("gen/chrome/vr_shell_resources.pak"));

  TabItemTexture texture;
  texture.SetSize(0.58f, 0.46f);
  texture.SetTitle(base::ASCIIToUTF16("test"));
  texture.SetColor(SK_ColorRED);
  texture.SetImageId(IDR_VR_SHELL_TAB_MOCK_0);

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
