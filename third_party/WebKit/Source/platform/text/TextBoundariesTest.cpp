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
    const std::string& input,
    AppendTrailingWhitespace append_trailing_whitespace =
        AppendTrailingWhitespace::kDontAppend) {
  // TODO(yosin): We should compute offset in UTF-16 instead of UTF-8.
  const size_t position = input.find('|');
  DCHECK_NE(position, std::string::npos)
      << "You should specify position marker as '|'.";
  std::string text = input;
  text.replace(position, 1, "");
  String text16 = String::FromUTF8(text.data(), text.size());
  text16.Ensure16Bit();
  const std::pair<int, int>& range =
      FindWordBoundary(text16.Characters16(), text16.length(), position,
                       append_trailing_whitespace);
  const int start = range.first;
  const int end = range.second;
  if (start < 0 && end < 0)
    return "";
  if (start < 0)
    return text.substr(0, end) + '|' + text.substr(end);
  if (end < 0)
    return text.substr(0, start) + '^' + text.substr(start);
  DCHECK_LE(start, end);
  return text.substr(0, start) + '^' + text.substr(start, end - start) + '|' +
         text.substr(end);
}

std::string TryAppendTrailingWhitespace(const std::string& input) {
  return TryFindWordBoundary(input, AppendTrailingWhitespace::kShouldAppend);
}

}  // namespace

TEST_F(TextBoundariesTest, AppendTrailingWhitespace) {
  EXPECT_EQ("^abc|", TryAppendTrailingWhitespace("a|bc"));
  EXPECT_EQ("^abc |xyz", TryAppendTrailingWhitespace("a|bc xyz"));
  EXPECT_EQ("^abc\n|xyz", TryAppendTrailingWhitespace("a|bc\nxyz"));
  EXPECT_EQ("^abc\xC2\xA0|xyz", TryAppendTrailingWhitespace("a|bc\xC2\xA0xyz"))
      << "U+00A0 is a whitespace";
  EXPECT_EQ("^abc  |xyz", TryAppendTrailingWhitespace("a|bc  xyz"))
      << "appends multiple whitesapces";
}

TEST_F(TextBoundariesTest, One) {
  EXPECT_EQ("^a|", TryFindWordBoundary("|a"));
  EXPECT_EQ("^a|", TryFindWordBoundary("a|"));
}

TEST_F(TextBoundariesTest, Parenthesis) {
  EXPECT_EQ("^(|abc)", TryFindWordBoundary("|(abc)"));
  EXPECT_EQ("(^abc|)", TryFindWordBoundary("(|abc)"));
  EXPECT_EQ("(^abc|)", TryFindWordBoundary("(a|bc)"));
  EXPECT_EQ("(^abc|)", TryFindWordBoundary("(ab|c)"));
  EXPECT_EQ("(abc^)|", TryFindWordBoundary("(abc)|"));
}

TEST_F(TextBoundariesTest, Punctuations) {
  EXPECT_EQ("^abc|,,", TryFindWordBoundary("|abc,,"));
  EXPECT_EQ("abc^,|,", TryFindWordBoundary("abc|,,"));
}

TEST_F(TextBoundariesTest, Whitespaces) {
  EXPECT_EQ("^ | abc  xyz  ", TryFindWordBoundary("|  abc  xyz  "));
  EXPECT_EQ(" ^ |abc  xyz  ", TryFindWordBoundary(" | abc  xyz  "));
  EXPECT_EQ("  ^abc|  xyz  ", TryFindWordBoundary("  |abc  xyz  "));
  EXPECT_EQ("  ^abc|  xyz  ", TryFindWordBoundary("  a|bc  xyz  "));
  EXPECT_EQ("  ^abc|  xyz  ", TryFindWordBoundary("  ab|c  xyz  "));
  EXPECT_EQ("  abc^ | xyz  ", TryFindWordBoundary("  abc|  xyz  "));
  EXPECT_EQ("  abc ^ |xyz  ", TryFindWordBoundary("  abc | xyz  "));
  EXPECT_EQ("  abc  ^xyz|  ", TryFindWordBoundary("  abc  |xyz  "));
  EXPECT_EQ("  abc  ^xyz|  ", TryFindWordBoundary("  abc  x|yz  "));
  EXPECT_EQ("  abc  ^xyz|  ", TryFindWordBoundary("  abc  xy|z  "));
  EXPECT_EQ("  abc  xyz^ | ", TryFindWordBoundary("  abc  xyz|  "));
  EXPECT_EQ("  abc  xyz ^ |", TryFindWordBoundary("  abc  xyz | "));
  EXPECT_EQ("  abc  xyz ^ |", TryFindWordBoundary("  abc  xyz  |"));
}

TEST_F(TextBoundariesTest, Zero) {
  EXPECT_EQ("|", TryFindWordBoundary("|"));
}

}  // namespace blink
