// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/i18n/bidi_line_iterator.h"

#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
namespace i18n {
namespace {

class BiDiLineIteratorTest : public testing::TestWithParam<TextDirection> {
 public:
  BiDiLineIterator* iterator() { return &iterator_; }

 private:
  BiDiLineIterator iterator_;
};

TEST_P(BiDiLineIteratorTest, OnlyLTR) {
  iterator()->Open(UTF8ToUTF16("abc 😁 测试"), GetParam(),
                   BiDiLineIterator::CustomBehavior::NONE);
  ASSERT_EQ(1, iterator()->CountRuns());

  int start, length;
  EXPECT_EQ(UBIDI_LTR, iterator()->GetVisualRun(0, &start, &length));
  EXPECT_EQ(0, start);
  EXPECT_EQ(9, length);

  int end;
  UBiDiLevel level;
  iterator()->GetLogicalRun(0, &end, &level);
  EXPECT_EQ(9, end);
  if (GetParam() == TextDirection::RIGHT_TO_LEFT)
    EXPECT_EQ(2, level);
  else
    EXPECT_EQ(0, level);
}

TEST_P(BiDiLineIteratorTest, OnlyRTL) {
  iterator()->Open(UTF8ToUTF16("מה השעה"), GetParam(),
                   BiDiLineIterator::CustomBehavior::NONE);
  ASSERT_EQ(1, iterator()->CountRuns());

  int start, length;
  EXPECT_EQ(UBIDI_RTL, iterator()->GetVisualRun(0, &start, &length));
  EXPECT_EQ(0, start);
  EXPECT_EQ(7, length);

  int end;
  UBiDiLevel level;
  iterator()->GetLogicalRun(0, &end, &level);
  EXPECT_EQ(7, end);
  EXPECT_EQ(1, level);
}

TEST_P(BiDiLineIteratorTest, Mixed) {
  iterator()->Open(UTF8ToUTF16("אני משתמש ב- Chrome כדפדפן האינטרנט שלי"),
                   GetParam(), BiDiLineIterator::CustomBehavior::NONE);
  ASSERT_EQ(3, iterator()->CountRuns());

  // We'll get completely different results depending on the top-level paragraph
  // direction.
  if (GetParam() == TextDirection::RIGHT_TO_LEFT) {
    // If para direction is RTL, expect the LTR substring "Chrome" to be nested
    // within the surrounding RTL text.
    int start, length;
    EXPECT_EQ(UBIDI_RTL, iterator()->GetVisualRun(0, &start, &length));
    EXPECT_EQ(19, start);
    EXPECT_EQ(20, length);
    EXPECT_EQ(UBIDI_LTR, iterator()->GetVisualRun(1, &start, &length));
    EXPECT_EQ(13, start);
    EXPECT_EQ(6, length);
    EXPECT_EQ(UBIDI_RTL, iterator()->GetVisualRun(2, &start, &length));
    EXPECT_EQ(0, start);
    EXPECT_EQ(13, length);

    int end;
    UBiDiLevel level;
    iterator()->GetLogicalRun(0, &end, &level);
    EXPECT_EQ(13, end);
    EXPECT_EQ(1, level);
    iterator()->GetLogicalRun(13, &end, &level);
    EXPECT_EQ(19, end);
    EXPECT_EQ(2, level);
    iterator()->GetLogicalRun(19, &end, &level);
    EXPECT_EQ(39, end);
    EXPECT_EQ(1, level);
  } else {
    // If the para direction is LTR, expect the LTR substring "- Chrome " to be
    // at the top level, with two nested RTL runs on either side.
    int start, length;
    EXPECT_EQ(UBIDI_RTL, iterator()->GetVisualRun(0, &start, &length));
    EXPECT_EQ(0, start);
    EXPECT_EQ(11, length);
    EXPECT_EQ(UBIDI_LTR, iterator()->GetVisualRun(1, &start, &length));
    EXPECT_EQ(11, start);
    EXPECT_EQ(9, length);
    EXPECT_EQ(UBIDI_RTL, iterator()->GetVisualRun(2, &start, &length));
    EXPECT_EQ(20, start);
    EXPECT_EQ(19, length);

    int end;
    UBiDiLevel level;
    iterator()->GetLogicalRun(0, &end, &level);
    EXPECT_EQ(11, end);
    EXPECT_EQ(1, level);
    iterator()->GetLogicalRun(11, &end, &level);
    EXPECT_EQ(20, end);
    EXPECT_EQ(0, level);
    iterator()->GetLogicalRun(20, &end, &level);
    EXPECT_EQ(39, end);
    EXPECT_EQ(1, level);
  }
}

INSTANTIATE_TEST_CASE_P(,
                        BiDiLineIteratorTest,
                        ::testing::Values(TextDirection::LEFT_TO_RIGHT,
                                          TextDirection::RIGHT_TO_LEFT));

}  // namespace
}  // namespace i18n
}  // namespace base
