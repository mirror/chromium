// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/fonts/Font.h"
#include "platform/fonts/FontDescription.h"
#include "platform/testing/FontTestHelpers.h"
#include "platform/testing/UnitTestHelpers.h"
#include "public/platform/FilePathConversion.h"
#include "public/platform/WebString.h"
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
                 int start = 0,
                 int end = -1) {
    FontDescription::VariantLigatures ligatures(
        FontDescription::kEnabledLigaturesState);
    Font font = CreateTestFont(
        "TestFont",
        FilePathToWebString(
            WebStringToFilePath(testing::BlinkRootDir())
                .Append(WebStringToFilePath(font_path[font_name]))),
        100, &ligatures);
    TextRun text_run(
        text, /* xpos */ 0, /* expansion */ 0,
        TextRun::kAllowTrailingExpansion | TextRun::kForbidLeadingExpansion,
        ltr ? TextDirection::kLtr : TextDirection::kRtl, false);

    if (end == -1)
      end = text_run.length();
    DCHECK_GE(start, 0);
    DCHECK_LE(start, static_cast<int>(text_run.length()));
    DCHECK_GE(end, -1);
    DCHECK_LE(end, static_cast<int>(text_run.length()));

    FloatRect rect =
        font.SelectionRectForText(text_run, FloatPoint(), 12, start, end);
    return rect.Width();
  }

  int GetCharacter(FontName font_name,
                   const String& text,
                   bool ltr,
                   float position,
                   bool partial) {
    FontDescription::VariantLigatures ligatures(
        FontDescription::kEnabledLigaturesState);
    Font font = CreateTestFont(
        "TestFont",
        FilePathToWebString(
            WebStringToFilePath(testing::BlinkRootDir())
                .Append(WebStringToFilePath(font_path[font_name]))),
        100, &ligatures);
    TextRun text_run(
        text, /* xpos */ 0, /* expansion */ 0,
        TextRun::kAllowTrailingExpansion | TextRun::kForbidLeadingExpansion,
        ltr ? TextDirection::kLtr : TextDirection::kRtl, false);

    return font.OffsetForPosition(text_run, position, partial);
  }

 private:
  std::map<FontName, WebString> font_path = {
      {ahem, "LayoutTests/resources/Ahem.woff"},
      {amiri,
       "Source/platform/testing/data/third_party/Amiri/amiri_arabic.woff2"},
      {megalopolis,
       "LayoutTests/third_party/MEgalopolis/MEgalopolisExtra.woff"},
      {roboto, "LayoutTests/third_party/Roboto/roboto-regular.woff2"},
  };
};

TEST_F(CursorPositionTest, CursorPositionInLigature) {
  const float kFUWidth = GetWidth(megalopolis, "FU", true);
  const float kRAWidth = GetWidth(megalopolis, "RA", true);

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

  const float kFFIWidth = GetWidth(roboto, "ffi", true);
  const float kFFWidth = GetWidth(roboto, "ff", true);
  const float kIWidth = GetWidth(roboto, u"î", true);

  EXPECT_NEAR(GetWidth(roboto, "ffi", true, 0, 1), kFFIWidth / 3.0, 1.0);
  EXPECT_NEAR(GetWidth(roboto, "ffi", true, 0, 2), kFFIWidth * 2.0 / 3.0, 1.0);
  EXPECT_NEAR(GetWidth(roboto, "ffi", true, 0, 3), kFFIWidth, 1.0);
  EXPECT_NEAR(GetWidth(roboto, "ffi", true, 1, 2), kFFIWidth / 3.0, 1.0);
  EXPECT_NEAR(GetWidth(roboto, "ffi", true, 1, 3), kFFIWidth * 2.0 / 3.0, 1.0);
  EXPECT_NEAR(GetWidth(roboto, "ffi", true, 2, 3), kFFIWidth / 3.0, 1.0);

  EXPECT_NEAR(GetWidth(roboto, u"ffî", true, 0, 1), kFFWidth / 2.0, 1.0);
  EXPECT_NEAR(GetWidth(roboto, u"ffî", true, 0, 2), kFFWidth, 1.0);
  EXPECT_NEAR(GetWidth(roboto, u"ffî", true, 0, 3), kFFWidth + kIWidth, 1.0);
  EXPECT_NEAR(GetWidth(roboto, u"ffî", true, 1, 2), kFFWidth / 2.0, 1.0);
  EXPECT_NEAR(GetWidth(roboto, u"ffî", true, 1, 3), kFFWidth / 2.0 + kIWidth,
              1.0);
  EXPECT_NEAR(GetWidth(roboto, u"ffî", true, 2, 3), kIWidth, 1.0);
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
  // The widths below are from the final shaped version, not from the single
  // characters. They were extracted with "hb-shape --font-size=100"

  EXPECT_EQ(GetWidth(amiri, u"ت", false, 0, 1), 93);

  const float kAboveKhaWidth = 55;
  const float kAboveTaWidth = 10;
  EXPECT_EQ(GetWidth(amiri, u"تخ", false, 0, 1), kAboveKhaWidth);
  EXPECT_EQ(GetWidth(amiri, u"تخ", false, 0, 2),
            kAboveKhaWidth + kAboveTaWidth);
  EXPECT_EQ(GetWidth(amiri, u"تخ", false, 1, 2), kAboveTaWidth);

  const float kTaWidth = 75;
  const float kKhaWidth = 7;
  const float kLamWidth = 56;
  const float kAlifWidth = 22;
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 0, 1), kAlifWidth);
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 0, 2), kAlifWidth + kLamWidth);
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 0, 3),
            kAlifWidth + kLamWidth + kKhaWidth);
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 0, 4),
            kAlifWidth + kLamWidth + kKhaWidth + kTaWidth);
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 1, 2), kLamWidth);
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 1, 3), kLamWidth + kKhaWidth);
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 1, 4),
            kLamWidth + kKhaWidth + kTaWidth);
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 2, 3), kKhaWidth);
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 2, 4), kKhaWidth + kTaWidth);
  EXPECT_EQ(GetWidth(amiri, u"الخط", false, 3, 4), kTaWidth);
}

TEST_F(CursorPositionTest, RTLLigature) {
  const float kFUWidth = GetWidth(megalopolis, "FU", true);
  const float kRAWidth = GetWidth(megalopolis, "RA", true);

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
