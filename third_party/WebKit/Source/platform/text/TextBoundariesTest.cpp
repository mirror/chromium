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

// Returns word boundray with start(^) and end(|) markes from text with
// position(|) marker.
std::string TryFindWordBoundary(
    const std::string input8,
    AppendTrailingWhitespace append_trailing_whitespace =
        AppendTrailingWhitespace::kDontAppend) {
  String input16 = String::FromUTF8(input8.data(), input8.size());
  input16.Ensure16Bit();
  const size_t position = input16.find('|');
  DCHECK(position != kNotFound) << input8 << " should have position marker(|).";
  String text16 = input16.Left(position);
  text16.append(input16.Substring(position + 1));
  text16.Ensure16Bit();
  std::pair<int, int> start_and_end =
      FindWordBoundary(text16.Characters16(), text16.length(), position,
                       append_trailing_whitespace);
  const int start = start_and_end.first;
  const int end = start_and_end.second;
  StringBuilder builder;
  if (start < 0 && end < 0) {
    builder.Append(text16);
  } else if (start < 0) {
    builder.Append(text16.Left(end));
    builder.Append('^');
    builder.Append(text16.Substring(end));
  } else if (end < 0) {
    builder.Append(text16.Left(start));
    builder.Append('|');
    builder.Append(text16.Substring(start));
  } else {
    builder.Append(text16.Left(start));
    builder.Append('^');
    builder.Append(text16.Substring(start, end - start));
    builder.Append('|');
  }
  builder.Append(text16.Substring(end));
  CString result8 = builder.ToString().Utf8();
  return std::string(result8.data(), result8.length());
}

}  // namespace

TEST_F(TextBoundariesTest, AppendTrailingWhitespace) {
  EXPECT_EQ("^abc  |def",
            TryFindWordBoundary("a|bc  def",
                                AppendTrailingWhitespace::kShouldAppend));
  EXPECT_EQ("^abc\n |def",
            TryFindWordBoundary("a|bc\n def",
                                AppendTrailingWhitespace::kShouldAppend));
  EXPECT_EQ("^abc \n|def",
            TryFindWordBoundary("a|bc \ndef",
                                AppendTrailingWhitespace::kShouldAppend));
  EXPECT_EQ("^abc\xC2\xA0 |def",
            TryFindWordBoundary("a|bc\xC2\xA0 def",
                                AppendTrailingWhitespace::kShouldAppend))
      << "C2 A2 is U+00A0 NBSP";
}

TEST_F(TextBoundariesTest, BiDi) {
  EXPECT_EQ("^\xD8\xA0\xD8\xA1\xD8\xA2| \xD8\xA3\xD8\xA4\xD8\xA5",
            TryFindWordBoundary(
                "|\xD8\xA0\xD8\xA1\xD8\xA2 \xD8\xA3\xD8\xA4\xD8\xA5"));
  EXPECT_EQ("^\xD8\xA0\xD8\xA1\xD8\xA2| \xD8\xA3\xD8\xA4\xD8\xA5",
            TryFindWordBoundary(
                "\xD8\xA0|\xD8\xA1\xD8\xA2 \xD8\xA3\xD8\xA4\xD8\xA5"));
  EXPECT_EQ("^\xD8\xA0\xD8\xA1\xD8\xA2| \xD8\xA3\xD8\xA4\xD8\xA5",
            TryFindWordBoundary(
                "\xD8\xA0\xD8\xA1|\xD8\xA2 \xD8\xA3\xD8\xA4\xD8\xA5"));
  EXPECT_EQ("\xD8\xA0\xD8\xA1\xD8\xA2^ |\xD8\xA3\xD8\xA4\xD8\xA5",
            TryFindWordBoundary(
                "\xD8\xA0\xD8\xA1\xD8\xA2| \xD8\xA3\xD8\xA4\xD8\xA5"));
  EXPECT_EQ("\xD8\xA0\xD8\xA1\xD8\xA2 ^\xD8\xA3\xD8\xA4\xD8\xA5|",
            TryFindWordBoundary(
                "\xD8\xA0\xD8\xA1\xD8\xA2 |\xD8\xA3\xD8\xA4\xD8\xA5"));
  EXPECT_EQ("\xD8\xA0\xD8\xA1\xD8\xA2 ^|\xD8\xA3\xD8\xA4\xD8\xA5",
            TryFindWordBoundary(
                "\xD8\xA0\xD8\xA1\xD8\xA2 \xD8\xA3\xD8\xA4\xD8\xA5|"));
}

TEST_F(TextBoundariesTest, BiDiMixed) {
  EXPECT_EQ("^abc\xD8\xA0\xD8\xA1\xD8\xA2|",
            TryFindWordBoundary("|abc\xD8\xA0\xD8\xA1\xD8\xA2"));
  EXPECT_EQ("^abc\xD8\xA0\xD8\xA1\xD8\xA2|",
            TryFindWordBoundary("ab|c\xD8\xA0\xD8\xA1\xD8\xA2"));
  EXPECT_EQ("^abc\xD8\xA0\xD8\xA1\xD8\xA2|",
            TryFindWordBoundary("abc|\xD8\xA0\xD8\xA1\xD8\xA2"))
      << "At L1/L2 boundary";
  EXPECT_EQ("^abc\xD8\xA0\xD8\xA1\xD8\xA2|",
            TryFindWordBoundary("abc\xD8\xA0|\xD8\xA1\xD8\xA2"));

  EXPECT_EQ("^\xD8\xA0\xD8\xA1\xD8\xA2xyz|",
            TryFindWordBoundary("|\xD8\xA0\xD8\xA1\xD8\xA2xyz"));
  EXPECT_EQ("^\xD8\xA0\xD8\xA1\xD8\xA2xyz|",
            TryFindWordBoundary("\xD8\xA0|\xD8\xA1\xD8\xA2xyz"));
  EXPECT_EQ("^\xD8\xA0\xD8\xA1\xD8\xA2xyz|",
            TryFindWordBoundary("\xD8\xA0\xD8\xA1\xD8\xA2|xyz"))
      << "At L2/L1 boundary";
  EXPECT_EQ("^\xD8\xA0\xD8\xA1\xD8\xA2xyz|",
            TryFindWordBoundary("\xD8\xA0\xD8\xA1\xD8\xA2xy|z"));
}

TEST_F(TextBoundariesTest, One) {
  EXPECT_EQ("^a|", TryFindWordBoundary("|a"));
  EXPECT_EQ("^|a", TryFindWordBoundary("a|")) << "No word after |";
}

TEST_F(TextBoundariesTest, Parenthesis) {
  EXPECT_EQ("^(|abc)", TryFindWordBoundary("|(abc)"));
  EXPECT_EQ("(^abc|)", TryFindWordBoundary("(|abc)"));
  EXPECT_EQ("(^abc|)", TryFindWordBoundary("(a|bc)"));
  EXPECT_EQ("(^abc|)", TryFindWordBoundary("(ab|c)"));
  EXPECT_EQ("(abc^|)", TryFindWordBoundary("(abc)|")) << "No word after |";
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
  EXPECT_EQ("  abc  def ^| ", TryFindWordBoundary("  abc  def  |"))
      << "No word after |";
}

TEST_F(TextBoundariesTest, Zero) {
  EXPECT_EQ("^|", TryFindWordBoundary("|"));
}

}  // namespace blink
