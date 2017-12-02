// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/strcat.h"
#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

// Each different argument has a different length to catch typos where the
// length of a variable is transposed with another line in the implementation.
TEST(StrCat, 8Bit) {
  EXPECT_EQ("122", StrCat("1", "22"));
  EXPECT_EQ("122333", StrCat("1", "22", "333"));
  EXPECT_EQ("1223334444", StrCat("1", "22", "333", "4444"));
  EXPECT_EQ("122333444455555", StrCat("1", "22", "333", "4444", "55555"));
  EXPECT_EQ("122333444455555666666",
            StrCat("1", "22", "333", "4444", "55555", "666666"));
  EXPECT_EQ("1223334444555556666667777777",
            StrCat("1", "22", "333", "4444", "55555", "666666", "7777777"));
  EXPECT_EQ("122333444455555666666777777788888888",
            StrCat("1", "22", "333", "4444", "55555", "666666", "7777777",
                   "88888888"));
  EXPECT_EQ("122333444455555666666777777788888888999999999",
            StrCat("1", "22", "333", "4444", "55555", "666666", "7777777",
                   "88888888", "999999999"));
  EXPECT_EQ("122333444455555666666777777788888888999999999aaaaaaaaaa",
            StrCat("1", "22", "333", "4444", "55555", "666666", "7777777",
                   "88888888", "999999999", "aaaaaaaaaa"));
}

TEST(StrCat, 16Bit) {
  string16 arg1 = ASCIIToUTF16("1");
  string16 arg2 = ASCIIToUTF16("22");
  string16 arg3 = ASCIIToUTF16("333");
  string16 arg4 = ASCIIToUTF16("4444");
  string16 arg5 = ASCIIToUTF16("55555");
  string16 arg6 = ASCIIToUTF16("666666");
  string16 arg7 = ASCIIToUTF16("7777777");
  string16 arg8 = ASCIIToUTF16("88888888");
  string16 arg9 = ASCIIToUTF16("999999999");
  string16 arg10 = ASCIIToUTF16("aaaaaaaaaa");

  EXPECT_EQ(ASCIIToUTF16("122333"), StrCat(arg1, arg2, arg3));
  EXPECT_EQ(ASCIIToUTF16("1223334444"), StrCat(arg1, arg2, arg3, arg4));
  EXPECT_EQ(ASCIIToUTF16("122333444455555"),
            StrCat(arg1, arg2, arg3, arg4, arg5));
  EXPECT_EQ(ASCIIToUTF16("122333444455555666666"),
            StrCat(arg1, arg2, arg3, arg4, arg5, arg6));
  EXPECT_EQ(ASCIIToUTF16("1223334444555556666667777777"),
            StrCat(arg1, arg2, arg3, arg4, arg5, arg6, arg7));
  EXPECT_EQ(ASCIIToUTF16("122333444455555666666777777788888888"),
            StrCat(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8));
  EXPECT_EQ(ASCIIToUTF16("122333444455555666666777777788888888999999999"),
            StrCat(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9));
  EXPECT_EQ(
      ASCIIToUTF16("122333444455555666666777777788888888999999999aaaaaaaaaa"),
      StrCat(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10));
}

TEST(StrCat, LotsOfArgs) {
  EXPECT_EQ(
      "122333444455555666666777777788888888999999999aaaaaaaaaabbbbbbbbbbbccc"
      "ccccccccc",
      StrCat("1", "22", "333", "4444", "55555", "666666", "7777777", "88888888",
             "999999999", "aaaaaaaaaa", "bbbbbbbbbbb", "cccccccccccc"));

  EXPECT_EQ(ASCIIToUTF16("122333444455555666666777777788888888999999999aaaaa"
                         "aaaaabbbbbbbbbbbcccccccccccc"),
            StrCat(ASCIIToUTF16("1"), ASCIIToUTF16("22"), ASCIIToUTF16("333"),
                   ASCIIToUTF16("4444"), ASCIIToUTF16("55555"),
                   ASCIIToUTF16("666666"), ASCIIToUTF16("7777777"),
                   ASCIIToUTF16("88888888"), ASCIIToUTF16("999999999"),
                   ASCIIToUTF16("aaaaaaaaaa"), ASCIIToUTF16("bbbbbbbbbbb"),
                   ASCIIToUTF16("cccccccccccc")));
}

TEST(StrAppend, 8Bit) {
  std::string result;

  result = "foo";
  StrAppend(&result, "1", "22");
  EXPECT_EQ("foo122", result);

  result = "foo";
  StrAppend(&result, "1", "22", "333");
  EXPECT_EQ("foo122333", result);

  result = "foo";
  StrAppend(&result, "1", "22", "333", "4444");
  EXPECT_EQ("foo1223334444", result);

  result = "foo";
  StrAppend(&result, "1", "22", "333", "4444", "55555");
  EXPECT_EQ("foo122333444455555", result);

  result = "foo";
  StrAppend(&result, "1", "22", "333", "4444", "55555", "666666");
  EXPECT_EQ("foo122333444455555666666", result);

  result = "foo";
  StrAppend(&result, "1", "22", "333", "4444", "55555", "666666", "7777777");
  EXPECT_EQ("foo1223334444555556666667777777", result);

  result = "foo";
  StrAppend(&result, "1", "22", "333", "4444", "55555", "666666", "7777777",
            "88888888");
  EXPECT_EQ("foo122333444455555666666777777788888888", result);

  result = "foo";
  StrAppend(&result, "1", "22", "333", "4444", "55555", "666666", "7777777",
            "88888888", "999999999");
  EXPECT_EQ("foo122333444455555666666777777788888888999999999", result);

  result = "foo";
  StrAppend(&result, "1", "22", "333", "4444", "55555", "666666", "7777777",
            "88888888", "999999999", "aaaaaaaaaa");
  EXPECT_EQ("foo122333444455555666666777777788888888999999999aaaaaaaaaa",
            result);
}

TEST(StrAppend, 16Bit) {
  string16 arg1 = ASCIIToUTF16("1");
  string16 arg2 = ASCIIToUTF16("22");
  string16 arg3 = ASCIIToUTF16("333");
  string16 arg4 = ASCIIToUTF16("4444");
  string16 arg5 = ASCIIToUTF16("55555");
  string16 arg6 = ASCIIToUTF16("666666");
  string16 arg7 = ASCIIToUTF16("7777777");
  string16 arg8 = ASCIIToUTF16("88888888");
  string16 arg9 = ASCIIToUTF16("999999999");
  string16 arg10 = ASCIIToUTF16("aaaaaaaaaa");

  string16 result;

  result = ASCIIToUTF16("foo");
  StrAppend(&result, arg1, arg2);
  EXPECT_EQ(ASCIIToUTF16("foo122"), result);

  result = ASCIIToUTF16("foo");
  StrAppend(&result, arg1, arg2, arg3);
  EXPECT_EQ(ASCIIToUTF16("foo122333"), result);

  result = ASCIIToUTF16("foo");
  StrAppend(&result, arg1, arg2, arg3, arg4);
  EXPECT_EQ(ASCIIToUTF16("foo1223334444"), result);

  result = ASCIIToUTF16("foo");
  StrAppend(&result, arg1, arg2, arg3, arg4, arg5);
  EXPECT_EQ(ASCIIToUTF16("foo122333444455555"), result);

  result = ASCIIToUTF16("foo");
  StrAppend(&result, arg1, arg2, arg3, arg4, arg5, arg6);
  EXPECT_EQ(ASCIIToUTF16("foo122333444455555666666"), result);

  result = ASCIIToUTF16("foo");
  StrAppend(&result, arg1, arg2, arg3, arg4, arg5, arg6, arg7);
  EXPECT_EQ(ASCIIToUTF16("foo1223334444555556666667777777"), result);

  result = ASCIIToUTF16("foo");
  StrAppend(&result, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);
  EXPECT_EQ(ASCIIToUTF16("foo122333444455555666666777777788888888"), result);

  result = ASCIIToUTF16("foo");
  StrAppend(&result, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9);
  EXPECT_EQ(ASCIIToUTF16("foo122333444455555666666777777788888888999999999"),
            result);

  result = ASCIIToUTF16("foo");
  StrAppend(&result, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9,
            arg10);
  EXPECT_EQ(ASCIIToUTF16(
                "foo122333444455555666666777777788888888999999999aaaaaaaaaa"),
            result);
}

TEST(StrAppend, InitializerList) {
  std::string result;

  result = "foo";
  StrAppendInitializerList(&result, {});
  EXPECT_EQ("foo", result);

  result = "foo";
  StrAppendInitializerList(&result, {"1"});
  EXPECT_EQ("foo1", result);

  result = "foo";
  StrAppendInitializerList(&result, {"1", "22", "333", "4444"});
  EXPECT_EQ("foo1223334444", result);
}

TEST(StrAppend, LotsOfArgs) {
  std::string result = "foo";
  StrAppend(&result, "1", "22", "333", "4444", "55555", "666666", "7777777",
            "88888888", "999999999", "aaaaaaaaaa", "bbbbbbbbbbb",
            "cccccccccccc");
  EXPECT_EQ(
      "foo122333444455555666666777777788888888999999999aaaaaaaaaabbbbbbbbbbbccc"
      "ccccccccc",
      result);

  string16 result16 = ASCIIToUTF16("foo");
  StrAppend(&result16, ASCIIToUTF16("1"), ASCIIToUTF16("22"),
            ASCIIToUTF16("333"), ASCIIToUTF16("4444"), ASCIIToUTF16("55555"),
            ASCIIToUTF16("666666"), ASCIIToUTF16("7777777"),
            ASCIIToUTF16("88888888"), ASCIIToUTF16("999999999"),
            ASCIIToUTF16("aaaaaaaaaa"), ASCIIToUTF16("bbbbbbbbbbb"),
            ASCIIToUTF16("cccccccccccc"));
  EXPECT_EQ(ASCIIToUTF16("foo122333444455555666666777777788888888999999999aaaaa"
                         "aaaaabbbbbbbbbbbcccccccccccc"),
            result16);
}

}  // namespace base
