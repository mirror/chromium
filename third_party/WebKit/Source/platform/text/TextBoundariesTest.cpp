// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "platform/text/TextBoundaries.h"

#include "platform/wtf/text/WTFString.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class TextBoundariesTest : public ::testing::Test {};

namespace {

// Returns word boundray with start(^) and end(|) markes from text with
// position(|) marker.
std::string TryFindWordBoundary(
    const std::string input,
    AppendTrailingWhitespace append_trailing_whitespace =
        AppendTrailingWhitespace::kDontAppend) {
  const size_t position = input.find('|');
  DCHECK_NE(position, std::string::npos)
      << "You should specify position marker as '|'.";
  std::string text = input;
  text.replace(position, 1, "");
  String text16 = String::FromUTF8(text.data(), text.size());
  text16.Ensure16Bit();
  std::pair<int, int> start_and_end =
      FindWordBoundary(text16.Characters16(), text16.length(), position,
                       append_trailing_whitespace);
  const int start = start_and_end.first;
  const int end = start_and_end.second;
  if (start < 0 && end < 0)
    return "";
  if (start < 0 || start == end)
    return text.substr(0, end) + '|' + text.substr(end);
  if (end < 0)
    return text.substr(0, start) + '^' + text.substr(start);
  DCHECK_LT(start, end);
  return text.substr(0, start) + '^' + text.substr(start, end - start) + '|' +
         text.substr(end);
}

}  // namespace

TEST_F(TextBoundariesTest, One) {
  EXPECT_EQ("^a|", TryFindWordBoundary("|a"));
  EXPECT_EQ("|a", TryFindWordBoundary("a|")) << "No word after |";
}

TEST_F(TextBoundariesTest, Parenthesis) {
  EXPECT_EQ("^(|abc)", TryFindWordBoundary("|(abc)"));
  EXPECT_EQ("(^abc|)", TryFindWordBoundary("(|abc)"));
  EXPECT_EQ("(^abc|)", TryFindWordBoundary("(a|bc)"));
  EXPECT_EQ("(^abc|)", TryFindWordBoundary("(ab|c)"));
  EXPECT_EQ("(abc|)", TryFindWordBoundary("(abc)|")) << "No word after |";
}

TEST_F(TextBoundariesTest, Punctuations) {
  EXPECT_EQ("^abc|,,", TryFindWordBoundary("|abc,,"));
  EXPECT_EQ("abc^,|,", TryFindWordBoundary("abc|,,"));
}

TEST_F(TextBoundariesTest, Whitespaces) {
  EXPECT_EQ("^ | abc  def  ", TryFindWordBoundary("|  abc  def  "));
  EXPECT_EQ(" ^ |abc  def  ", TryFindWordBoundary(" | abc  def  "));
  EXPECT_EQ("  ^abc|  def  ", TryFindWordBoundary("  |abc  def  "));
  EXPECT_EQ("  ^abc|  def  ", TryFindWordBoundary("  a|bc  def  "));
  EXPECT_EQ("  ^abc|  def  ", TryFindWordBoundary("  ab|c  def  "));
  EXPECT_EQ("  abc^ | def  ", TryFindWordBoundary("  abc|  def  "));
  EXPECT_EQ("  abc ^ |def  ", TryFindWordBoundary("  abc | def  "));
  EXPECT_EQ("  abc  ^def|  ", TryFindWordBoundary("  abc  |def  "));
  EXPECT_EQ("  abc  ^def|  ", TryFindWordBoundary("  abc  d|ef  "));
  EXPECT_EQ("  abc  ^def|  ", TryFindWordBoundary("  abc  de|f  "));
  EXPECT_EQ("  abc  def^ | ", TryFindWordBoundary("  abc  def|  "));
  EXPECT_EQ("  abc  def ^ |", TryFindWordBoundary("  abc  def | "));
  EXPECT_EQ("  abc  def | ", TryFindWordBoundary("  abc  def  |"))
      << "No word after |";
}

TEST_F(TextBoundariesTest, Zero) {
  EXPECT_EQ("|", TryFindWordBoundary("|"));
}

}  // namespace blink
