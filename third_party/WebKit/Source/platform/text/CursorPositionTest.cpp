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
  float GetWidth(const String& font_name,
                 const String& text,
                 bool ltr,
                 int start,
                 int end) {
    DVLOG(1) << font_name << ": " << text << (ltr ? "ltr" : "rtl") << " : "
             << start << "-" << end;
    FontDescription::VariantLigatures ligatures(
        FontDescription::kEnabledLigaturesState);
    Font font;
    if (font_name == "megalopolis") {
      font =
          CreateTestFont("MEgalopolis",
                         testing::PlatformTestDataPath(
                             "third_party/MEgalopolis/MEgalopolisExtra.woff"),
                         100, &ligatures);
    } else if (font_name == "amiri") {
      font = CreateTestFont(
          "Amiri",
          testing::PlatformTestDataPath("third_party/Amiri/amiri_arabic.woff2"),
          100, &ligatures);
    } else if (font_name == "ahem") {
      font = CreateTestFont("Ahem", testing::PlatformTestDataPath("Ahem.woff"),
                            100, &ligatures);
    }
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
};

TEST_F(CursorPositionTest, CursorPositionInLigature) {
  const float kFU = 86.0999;
  const float kRA = 115;

  EXPECT_EQ(GetWidth("megalopolis", "FURA", true, 0, 1), round(kFU / 2));
  EXPECT_EQ(GetWidth("megalopolis", "FURA", true, 0, 2), round(kFU));
  EXPECT_EQ(GetWidth("megalopolis", "FURA", true, 0, 3), round(kFU + kRA / 2));
  EXPECT_EQ(GetWidth("megalopolis", "FURA", true, 0, 4), round(kFU + kRA));

  EXPECT_EQ(GetWidth("megalopolis", "FURA", true, 1, 2), round(kFU / 2));
  EXPECT_EQ(GetWidth("megalopolis", "FURA", true, 1, 3),
            round(kFU / 2 + kRA / 2));
  EXPECT_EQ(GetWidth("megalopolis", "FURA", true, 1, 4), round(kFU / 2 + kRA));

  EXPECT_EQ(GetWidth("megalopolis", "FURA", true, 2, 3), round(kRA / 2));
  EXPECT_EQ(GetWidth("megalopolis", "FURA", true, 2, 4), round(kRA));

  EXPECT_EQ(GetWidth("megalopolis", "FURA", true, 3, 4), floor(kRA / 2));
}

TEST_F(CursorPositionTest, LTRText) {
  EXPECT_EQ(GetWidth("ahem", "X", true, 0, 1), 100);

  EXPECT_EQ(GetWidth("ahem", "XXX", true, 0, 1), 100);
  EXPECT_EQ(GetWidth("ahem", "XXX", true, 0, 2), 200);
  EXPECT_EQ(GetWidth("ahem", "XXX", true, 0, 3), 300);
  EXPECT_EQ(GetWidth("ahem", "XXX", true, 1, 2), 100);
  EXPECT_EQ(GetWidth("ahem", "XXX", true, 1, 3), 200);
  EXPECT_EQ(GetWidth("ahem", "XXX", true, 2, 3), 100);
}

TEST_F(CursorPositionTest, RTLText) {
  EXPECT_EQ(GetWidth("amiri", u"ت", false, 0, 1), 93);

  EXPECT_EQ(GetWidth("amiri", u"الخط", false, 0, 1), 22);
  EXPECT_EQ(GetWidth("amiri", u"الخط", false, 0, 2), 78);
  EXPECT_EQ(GetWidth("amiri", u"الخط", false, 0, 3), 85);
  EXPECT_EQ(GetWidth("amiri", u"الخط", false, 0, 4), 160);
  EXPECT_EQ(GetWidth("amiri", u"الخط", false, 1, 2), 56);
  EXPECT_EQ(GetWidth("amiri", u"الخط", false, 1, 3), 63);
  EXPECT_EQ(GetWidth("amiri", u"الخط", false, 1, 4), 138);
  EXPECT_EQ(GetWidth("amiri", u"الخط", false, 2, 3), 7);
  EXPECT_EQ(GetWidth("amiri", u"الخط", false, 2, 4), 82);
  EXPECT_EQ(GetWidth("amiri", u"الخط", false, 3, 4), 75);

  EXPECT_EQ(GetWidth("amiri", u"الأميري", false, 0, 1), 22);
  EXPECT_EQ(GetWidth("amiri", u"الأميري", false, 0, 2), 52);
  EXPECT_EQ(GetWidth("amiri", u"الأميري", false, 0, 3), 87);
  EXPECT_EQ(GetWidth("amiri", u"الأميري", false, 0, 4), 125);
  EXPECT_EQ(GetWidth("amiri", u"الأميري", false, 0, 5), 151);
  EXPECT_EQ(GetWidth("amiri", u"الأميري", false, 0, 6), 181);
  EXPECT_EQ(GetWidth("amiri", u"الأميري", false, 0, 7), 257);

  EXPECT_EQ(GetWidth("amiri", u"تخ", false, 0, 1), 55);
  EXPECT_EQ(GetWidth("amiri", u"تخ", false, 0, 2), 65);

  EXPECT_EQ(GetWidth("amiri", u"الله", false, 0, 1), 22);
  EXPECT_EQ(GetWidth("amiri", u"الله", false, 0, 2), 38);
  EXPECT_EQ(GetWidth("amiri", u"الله", false, 0, 3), 61);
  EXPECT_EQ(GetWidth("amiri", u"الله", false, 0, 4), 96);
  EXPECT_EQ(GetWidth("amiri", u"الله", false, 1, 2), 16);
  EXPECT_EQ(GetWidth("amiri", u"الله", false, 1, 3), 39);
  EXPECT_EQ(GetWidth("amiri", u"الله", false, 1, 4), 74);
  EXPECT_EQ(GetWidth("amiri", u"الله", false, 2, 3), 23);
  EXPECT_EQ(GetWidth("amiri", u"الله", false, 2, 4), 58);
  EXPECT_EQ(GetWidth("amiri", u"الله", false, 3, 4), 35);
}

}  // namespace blink
