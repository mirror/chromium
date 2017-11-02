// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "platform/fonts/Font.h"
#include "platform/fonts/FontDescription.h"
#include "platform/testing/FontTestHelpers.h"
#include "platform/testing/UnitTestHelpers.h"

using blink::testing::CreateTestFont;

namespace blink {

TEST(CursorPositionTest, CursorPositionInLigature) {
  FontDescription::VariantLigatures ligatures(FontDescription::kEnabledLigaturesState);
  Font font = CreateTestFont("MEgalopolis",
    testing::PlatformTestDataPath("third_party/MEgalopolis/MEgalopolisExtra.woff"),
    12, &ligatures);

  // FU, RA, CO and LO form ligatures in MEgalopolis.
  String ligature_test("FURACOLO");

  TextRun text_run(ligature_test, /* xpos */ 0, /* expansion */ 0,
          TextRun::kAllowTrailingExpansion | TextRun::kForbidLeadingExpansion,
          TextDirection::kLtr, false);
  text_run.SetNormalizeSpace(true);

  FloatPoint start_selection;
  FloatRect first_char = font.SelectionRectForText(text_run, start_selection, 16, 0, 1);
  FloatRect first_two_chars = font.SelectionRectForText(text_run, start_selection, 16, 0, 2);
  EXPECT_FALSE(first_char.Width() == first_two_chars.Width());
}

}  // namespace blink
