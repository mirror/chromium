// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/fonts/FontCache.h"
#include "platform/fonts/opentype/OS2TableCJKFont.h"

#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

#if defined(OS_WIN)

static void TestIsFontCJK(AtomicString windows_family_name, bool cjk_expected) {
  FontCache* font_cache = FontCache::GetFontCache();
  FontDescription font_description;
  FontFamily font_family;
  font_family.SetFamily(windows_family_name);
  font_description.SetFamily(font_family);
  RefPtr<SimpleFontData> simple_font_data =
      font_cache->GetFontData(font_description, windows_family_name);
  ASSERT_TRUE(simple_font_data);
  const FontPlatformData& font_platform_data = simple_font_data->PlatformData();
  ASSERT_TRUE(font_platform_data.Typeface());
  ASSERT_EQ(cjk_expected, OS2TableCJKFont::IsCJKFontFromOS2Table(
                              font_platform_data.Typeface()));
}

TEST(OS2TableCJKFontTest, Simsun) {
  TestIsFontCJK("Simsun", true);
}

TEST(OS2TableCJKFontTest, Calibri) {
  TestIsFontCJK("Calibri", false);
}

TEST(OS2TableCJKFontTest, SegoeUIEmoji) {
  TestIsFontCJK("Segoe UI", true);
}

TEST(OS2TableCJKFontTest, Pmingliuextb) {
  TestIsFontCJK("Pmingliu-extb", false);
}

#endif
}
