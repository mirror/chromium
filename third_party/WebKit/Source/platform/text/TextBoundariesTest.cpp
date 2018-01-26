// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "platform/text/TextBoundaries.h"

#include "platform/wtf/text/StringBuilder.h"
#include "platform/wtf/text/WTFString.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

class TextBoundariesTest : public ::testing::Test {};

namespace {

std::pair<String, int> ParsePositionMarker(const std::string input8) {
  String input16 = String::FromUTF8(input8.data(), input8.size());
  input16.Ensure16Bit();
  const size_t position = input16.find('|');
  DCHECK(position != kNotFound) << input8 << " should have position marker(|).";
  String text16 = input16.Left(position);
  text16.append(input16.Substring(position + 1));
  text16.Ensure16Bit();
  return {text16, position};
}

std::string MakeResultText(const String& text, int start, int end) {
  StringBuilder builder;
  if (start < 0 && end < 0) {
    builder.Append(text);
  } else if (start < 0) {
    builder.Append(text.Left(end));
    builder.Append('^');
    builder.Append(text.Substring(end));
  } else if (end < 0) {
    builder.Append(text.Left(start));
    builder.Append('|');
    builder.Append(text.Substring(start));
  } else {
    builder.Append(text.Left(start));
    builder.Append('^');
    builder.Append(text.Substring(start, end - start));
    builder.Append('|');
  }
  builder.Append(text.Substring(end));
  const CString result8 = builder.ToString().Utf8();
  return std::string(result8.data(), result8.length());
}

// Returns word boundray with start(^) and end(|) markes from text with
// position(|) marker.
std::string TryFindWordBackward(const std::string input8) {
  std::pair<String, int> string_and_offset = ParsePositionMarker(input8);
  const String text16 = string_and_offset.first;
  const int position = string_and_offset.second;
  std::pair<int, int> start_and_end =
      FindWordBackward(text16.Characters16(), text16.length(), position);
  return MakeResultText(text16, start_and_end.first, start_and_end.second);
}

// Returns word boundray with start(^) and end(|) markes from text with
// position(|) marker.
std::string TryFindWordForward(const std::string input8) {
  std::pair<String, int> string_and_offset = ParsePositionMarker(input8);
  const String text16 = string_and_offset.first;
  const int position = string_and_offset.second;
  std::pair<int, int> start_and_end =
      FindWordForward(text16.Characters16(), text16.length(), position);
  return MakeResultText(text16, start_and_end.first, start_and_end.second);
}

}  // namespace

TEST_F(TextBoundariesTest, BackwardBasic) {
  EXPECT_EQ("^|abc def", TryFindWordBackward("|abc def"));
  EXPECT_EQ("^abc| def", TryFindWordBackward("a|bc def"));
  EXPECT_EQ("^abc| def", TryFindWordBackward("ab|c def"));
  EXPECT_EQ("^abc| def", TryFindWordBackward("abc| def"));
  EXPECT_EQ("abc^ |def", TryFindWordBackward("abc |def"));
  EXPECT_EQ("abc ^def|", TryFindWordBackward("abc d|ef"));
  EXPECT_EQ("abc ^def|", TryFindWordBackward("abc de|f"));
  EXPECT_EQ("abc ^def|", TryFindWordBackward("abc def|"));
}

TEST_F(TextBoundariesTest, ForwardBasic) {
  EXPECT_EQ("^abc| def", TryFindWordForward("|abc def"));
  EXPECT_EQ("^abc| def", TryFindWordForward("a|bc def"));
  EXPECT_EQ("^abc| def", TryFindWordForward("ab|c def"));
  EXPECT_EQ("abc^ |def", TryFindWordForward("abc| def"));
  EXPECT_EQ("abc ^def|", TryFindWordForward("abc |def"));
  EXPECT_EQ("abc ^def|", TryFindWordForward("abc d|ef"));
  EXPECT_EQ("abc ^def|", TryFindWordForward("abc de|f"));
  EXPECT_EQ("abc def^|", TryFindWordForward("abc def|"));
}

TEST_F(TextBoundariesTest, ForwardBiDi) {
  EXPECT_EQ(
      "^\xD8\xA0\xD8\xA1\xD8\xA2| \xD8\xA3\xD8\xA4\xD8\xA5",
      TryFindWordForward("|\xD8\xA0\xD8\xA1\xD8\xA2 \xD8\xA3\xD8\xA4\xD8\xA5"));
  EXPECT_EQ(
      "^\xD8\xA0\xD8\xA1\xD8\xA2| \xD8\xA3\xD8\xA4\xD8\xA5",
      TryFindWordForward("\xD8\xA0|\xD8\xA1\xD8\xA2 \xD8\xA3\xD8\xA4\xD8\xA5"));
  EXPECT_EQ(
      "^\xD8\xA0\xD8\xA1\xD8\xA2| \xD8\xA3\xD8\xA4\xD8\xA5",
      TryFindWordForward("\xD8\xA0\xD8\xA1|\xD8\xA2 \xD8\xA3\xD8\xA4\xD8\xA5"));
  EXPECT_EQ(
      "\xD8\xA0\xD8\xA1\xD8\xA2^ |\xD8\xA3\xD8\xA4\xD8\xA5",
      TryFindWordForward("\xD8\xA0\xD8\xA1\xD8\xA2| \xD8\xA3\xD8\xA4\xD8\xA5"));
  EXPECT_EQ(
      "\xD8\xA0\xD8\xA1\xD8\xA2 ^\xD8\xA3\xD8\xA4\xD8\xA5|",
      TryFindWordForward("\xD8\xA0\xD8\xA1\xD8\xA2 |\xD8\xA3\xD8\xA4\xD8\xA5"));
  EXPECT_EQ(
      "\xD8\xA0\xD8\xA1\xD8\xA2 \xD8\xA3\xD8\xA4\xD8\xA5^|",
      TryFindWordForward("\xD8\xA0\xD8\xA1\xD8\xA2 \xD8\xA3\xD8\xA4\xD8\xA5|"));
}

TEST_F(TextBoundariesTest, ForwardBiDiMixed) {
  EXPECT_EQ("^abc\xD8\xA0\xD8\xA1\xD8\xA2|",
            TryFindWordForward("|abc\xD8\xA0\xD8\xA1\xD8\xA2"));
  EXPECT_EQ("^abc\xD8\xA0\xD8\xA1\xD8\xA2|",
            TryFindWordForward("ab|c\xD8\xA0\xD8\xA1\xD8\xA2"));
  EXPECT_EQ("^abc\xD8\xA0\xD8\xA1\xD8\xA2|",
            TryFindWordForward("abc|\xD8\xA0\xD8\xA1\xD8\xA2"))
      << "At L1/L2 boundary";
  EXPECT_EQ("^abc\xD8\xA0\xD8\xA1\xD8\xA2|",
            TryFindWordForward("abc\xD8\xA0|\xD8\xA1\xD8\xA2"));

  EXPECT_EQ("^\xD8\xA0\xD8\xA1\xD8\xA2xyz|",
            TryFindWordForward("|\xD8\xA0\xD8\xA1\xD8\xA2xyz"));
  EXPECT_EQ("^\xD8\xA0\xD8\xA1\xD8\xA2xyz|",
            TryFindWordForward("\xD8\xA0|\xD8\xA1\xD8\xA2xyz"));
  EXPECT_EQ("^\xD8\xA0\xD8\xA1\xD8\xA2xyz|",
            TryFindWordForward("\xD8\xA0\xD8\xA1\xD8\xA2|xyz"))
      << "At L2/L1 boundary";
  EXPECT_EQ("^\xD8\xA0\xD8\xA1\xD8\xA2xyz|",
            TryFindWordForward("\xD8\xA0\xD8\xA1\xD8\xA2xy|z"));
}

TEST_F(TextBoundariesTest, ForwardOne) {
  EXPECT_EQ("^a|", TryFindWordForward("|a"));
  EXPECT_EQ("a^|", TryFindWordForward("a|")) << "No word after |";
}

TEST_F(TextBoundariesTest, ForwardParenthesis) {
  EXPECT_EQ("^(|abc)", TryFindWordForward("|(abc)"));
  EXPECT_EQ("(^abc|)", TryFindWordForward("(|abc)"));
  EXPECT_EQ("(^abc|)", TryFindWordForward("(a|bc)"));
  EXPECT_EQ("(^abc|)", TryFindWordForward("(ab|c)"));
  EXPECT_EQ("(abc)^|", TryFindWordForward("(abc)|")) << "No word after |";
}

TEST_F(TextBoundariesTest, ForwardPunctuations) {
  EXPECT_EQ("^abc|,,", TryFindWordForward("|abc,,"));
  EXPECT_EQ("abc^,|,", TryFindWordForward("abc|,,"));
}

TEST_F(TextBoundariesTest, ForwardWhitespaces) {
  EXPECT_EQ("^ | abc  def  ", TryFindWordForward("|  abc  def  "));
  EXPECT_EQ(" ^ |abc  def  ", TryFindWordForward(" | abc  def  "));
  EXPECT_EQ("  ^abc|  def  ", TryFindWordForward("  |abc  def  "));
  EXPECT_EQ("  ^abc|  def  ", TryFindWordForward("  a|bc  def  "));
  EXPECT_EQ("  ^abc|  def  ", TryFindWordForward("  ab|c  def  "));
  EXPECT_EQ("  abc^ | def  ", TryFindWordForward("  abc|  def  "));
  EXPECT_EQ("  abc ^ |def  ", TryFindWordForward("  abc | def  "));
  EXPECT_EQ("  abc  ^def|  ", TryFindWordForward("  abc  |def  "));
  EXPECT_EQ("  abc  ^def|  ", TryFindWordForward("  abc  d|ef  "));
  EXPECT_EQ("  abc  ^def|  ", TryFindWordForward("  abc  de|f  "));
  EXPECT_EQ("  abc  def^ | ", TryFindWordForward("  abc  def|  "));
  EXPECT_EQ("  abc  def ^ |", TryFindWordForward("  abc  def | "));
  EXPECT_EQ("  abc  def  ^|", TryFindWordForward("  abc  def  |"))
      << "No word after |";
}

TEST_F(TextBoundariesTest, ForwardZero) {
  EXPECT_EQ("^|", TryFindWordForward("|"));
}

}  // namespace blink
