// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/fonts/Font.h"
#include "platform/fonts/FontDescription.h"
#include "platform/testing/FontTestHelpers.h"
#include "platform/testing/UnitTestHelpers.h"
#include "testing/gtest/include/gtest/gtest.h"

using blink::testing::CreateTestFont;

namespace blink {

class CursorPositionTest : public ::testing::Test {
 public:
  enum FontName {
    ahem,
    amiri,
    megalopolis,
    roboto,
  };

  float GetWidth(FontName font_name,
                 const String& text,
                 bool ltr,
                 int start,
                 int end) {
    DVLOG(1) << font_name << ": " << text << (ltr ? "ltr" : "rtl") << " : "
             << start << "-" << end;
    FontDescription::VariantLigatures ligatures(
        FontDescription::kEnabledLigaturesState);
    Font font = CreateTestFont(
        "TestFont", testing::PlatformTestDataPath(font_path[font_name]), 100,
        &ligatures);
    TextRun text_run(
        text, /* xpos */ 0, /* expansion */ 0,
        TextRun::kAllowTrailingExpansion | TextRun::kForbidLeadingExpansion,
        ltr ? TextDirection::kLtr : TextDirection::kRtl, false);

    DCHECK_GE(start, 0);
    DCHECK_LE(start, static_cast<int>(text_run.length()));
    DCHECK_GE(end, -1);
    DCHECK_LE(end, static_cast<int>(text_run.length()));

    FloatRect rect =
        font.SelectionRectForText(text_run, FloatPoint(), 12, start, end);
    return rect.Width();
  }

 private:
  std::map<FontName, String> font_path = {
      {ahem, "Ahem.woff"},
      {amiri, "third_party/Amiri/amiri_arabic.woff2"},
      {megalopolis, "third_party/MEgalopolis/MEgalopolisExtra.woff"},
      {roboto, "third_party/roboto-regular.woff2"},
  };
};

TEST_F(CursorPositionTest, CursorPositionInLigature) {
  const float kFUWidth = 86.0999;
  const float kRAWidth = 115;

  EXPECT_NEAR(GetWidth(megalopolis, "FURA", true, 0, 1), kFUWidth / 2, 1.0);
  EXPECT_NEAR(GetWidth(megalopolis, "FURA", true, 0, 2), kFUWidth, 1.0);
  EXPECT_NEAR(GetWidth(megalopolis, "FURA", true, 0, 3),
              kFUWidth + kRAWidth / 2, 1.0);
  EXPECT_NEAR(GetWidth(megalopolis, "FURA", true, 0, 4), kFUWidth + kRAWidth,
              1.0);

  EXPECT_NEAR(GetWidth(megalopolis, "FURA", true, 1, 2), kFUWidth / 2, 1.0);
  EXPECT_NEAR(GetWidth(megalopolis, "FURA", true, 1, 3),
              kFUWidth / 2 + kRAWidth / 2, 1.0);
  EXPECT_NEAR(GetWidth(megalopolis, "FURA", true, 1, 4),
              kFUWidth / 2 + kRAWidth, 1.0);

  EXPECT_NEAR(GetWidth(megalopolis, "FURA", true, 2, 3), kRAWidth / 2, 1.0);
  EXPECT_NEAR(GetWidth(megalopolis, "FURA", true, 2, 4), kRAWidth, 1.0);

  EXPECT_NEAR(GetWidth(megalopolis, "FURA", true, 3, 4), kRAWidth / 2, 1.0);

  const float kFFIWidth = 85.3516;
  const float kFFWidth = 61.8652;
  const float kIWidth = 65.2344;

  EXPECT_NEAR(GetWidth(roboto, "ffi", true, 0, 1), kFFIWidth / 3.0, 1.0);
  EXPECT_NEAR(GetWidth(roboto, "ffi", true, 0, 2), kFFIWidth * 2.0 / 3.0, 1.0);
  EXPECT_NEAR(GetWidth(roboto, "ffi", true, 0, 3), kFFIWidth, 1.0);
  EXPECT_NEAR(GetWidth(roboto, "ffi", true, 1, 2), kFFIWidth / 3.0, 1.0);
  EXPECT_NEAR(GetWidth(roboto, "ffi", true, 1, 3), kFFIWidth * 2.0 / 3.0, 1.0);
  EXPECT_NEAR(GetWidth(roboto, "ffi", true, 2, 3), kFFIWidth / 3.0, 1.0);

  EXPECT_NEAR(GetWidth(roboto, "ffî", true, 0, 1), kFFWidth / 2.0, 1.0);
  EXPECT_NEAR(GetWidth(roboto, "ffî", true, 0, 2), kFFWidth, 1.0);
  EXPECT_NEAR(GetWidth(roboto, "ffî", true, 0, 3), kFFWidth + kIWidth, 1.0);
  EXPECT_NEAR(GetWidth(roboto, "ffî", true, 1, 2), kFFWidth / 2.0, 1.0);
  EXPECT_NEAR(GetWidth(roboto, "ffî", true, 1, 3), kFFWidth / 2.0 + kIWidth,
              1.0);
  EXPECT_NEAR(GetWidth(roboto, "ffî", true, 2, 3), kIWidth, 1.0);
}

TEST_F(CursorPositionTest, LTRText) {
  EXPECT_EQ(GetWidth(ahem, "X", true, 0, 1), 100);

  EXPECT_EQ(GetWidth(ahem, "XXX", true, 0, 1), 100);
  EXPECT_EQ(GetWidth(ahem, "XXX", true, 0, 2), 200);
  EXPECT_EQ(GetWidth(ahem, "XXX", true, 0, 3), 300);
  EXPECT_EQ(GetWidth(ahem, "XXX", true, 1, 2), 100);
  EXPECT_EQ(GetWidth(ahem, "XXX", true, 1, 3), 200);
  EXPECT_EQ(GetWidth(ahem, "XXX", true, 2, 3), 100);
}

TEST_F(CursorPositionTest, RTLText) {
  EXPECT_EQ(GetWidth(amiri, u"ت", false, 0, 1), 93);

  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 0, 1), 22);
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 0, 2), 43);
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 0, 3), 65);
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 0, 4), 160);
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 1, 2), 21);
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 1, 3), 43);
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 1, 4), 138);
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 2, 3), 22);
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 2, 4), 117);
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 3, 4), 95);

  EXPECT_EQ(GetWidth(amiri, u"تخ", false, 0, 1), 55);
  EXPECT_EQ(GetWidth(amiri, u"تخ", false, 0, 2), 65);
}

TEST_F(CursorPositionTest, RTLLigature) {
  const float kFUWidth = 86.0999;
  const float kRAWidth = 115;

  EXPECT_NEAR(GetWidth(megalopolis, "ARUF", false, 0, 1), kRAWidth / 2, 1.0);
  EXPECT_NEAR(GetWidth(megalopolis, "ARUF", false, 0, 2), kRAWidth, 1.0);
  EXPECT_NEAR(GetWidth(megalopolis, "ARUF", false, 0, 3),
              kRAWidth + kFUWidth / 2, 1.0);
  EXPECT_NEAR(GetWidth(megalopolis, "ARUF", false, 0, 4), kRAWidth + kFUWidth,
              1.0);

  EXPECT_NEAR(GetWidth(megalopolis, "ARUF", false, 1, 2), kRAWidth / 2, 1.0);
  EXPECT_NEAR(GetWidth(megalopolis, "ARUF", false, 1, 3),
              kRAWidth / 2 + kFUWidth / 2, 1.0);
  EXPECT_NEAR(GetWidth(megalopolis, "ARUF", false, 1, 4),
              kRAWidth / 2 + kFUWidth, 1.0);

  EXPECT_NEAR(GetWidth(megalopolis, "ARUF", false, 2, 3), kFUWidth / 2, 1.0);
  EXPECT_NEAR(GetWidth(megalopolis, "ARUF", false, 2, 4), kFUWidth, 1.0);

  EXPECT_NEAR(GetWidth(megalopolis, "ARUF", false, 3, 4), kFUWidth / 2, 1.0);
}

}  // namespace blink
