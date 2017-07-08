// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/composition_text.h"

#include <stddef.h>

#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ui {

TEST(CompositionTextTest, CopyTest) {
  const base::string16 kSampleText = base::UTF8ToUTF16("Sample Text");
  const CompositionUnderline kSampleUnderline1(10, 20, SK_ColorBLACK, false,
                                               SK_ColorTRANSPARENT);

  const CompositionUnderline kSampleUnderline2(11, 21, SK_ColorBLACK, true,
                                               SK_ColorTRANSPARENT);

  const CompositionUnderline kSampleUnderline3(12, 22, SK_ColorRED, false,
                                               SK_ColorTRANSPARENT);

  // Make CompositionText
  CompositionText text;
  text.text = kSampleText;
  text.text_composition_data.composition_underlines.push_back(
      kSampleUnderline1);
  text.text_composition_data.composition_underlines.push_back(
      kSampleUnderline2);
  text.text_composition_data.composition_underlines.push_back(
      kSampleUnderline3);
  text.selection.set_start(30);
  text.selection.set_end(40);

  CompositionText text2;
  text2.CopyFrom(text);

  EXPECT_EQ(text.text, text2.text);
  EXPECT_EQ(text.text_composition_data.composition_underlines.size(),
            text2.text_composition_data.composition_underlines.size());
  for (size_t i = 0;
       i < text.text_composition_data.composition_underlines.size(); ++i) {
    EXPECT_EQ(
        text.text_composition_data.composition_underlines[i].start_offset,
        text2.text_composition_data.composition_underlines[i].start_offset);
    EXPECT_EQ(text.text_composition_data.composition_underlines[i].end_offset,
              text2.text_composition_data.composition_underlines[i].end_offset);
    EXPECT_EQ(text.text_composition_data.composition_underlines[i].color,
              text2.text_composition_data.composition_underlines[i].color);
    EXPECT_EQ(text.text_composition_data.composition_underlines[i].thick,
              text2.text_composition_data.composition_underlines[i].thick);
    EXPECT_EQ(
        text.text_composition_data.composition_underlines[i].background_color,
        text2.text_composition_data.composition_underlines[i].background_color);
  }

  EXPECT_EQ(text.selection.start(), text2.selection.start());
  EXPECT_EQ(text.selection.end(), text2.selection.end());
}

}  // namespace ui
