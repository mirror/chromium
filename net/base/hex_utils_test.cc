// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace test {

TEST(HexUtilsTest, HexDecode) {
  EXPECT_EQ("", HexDecode(""));
  EXPECT_EQ("a", HexDecode("61"));
  // Mixed case input.
  EXPECT_EQ("Hello world!", HexDecode("48656c6C6F20776f726C6421"));
}

TEST(HexUtilsTest, HexDump) {
  EXPECT_EQ("", HexDump(""));
  EXPECT_EQ("0x0000:  4865 6c6c 6f20 776f 726c 6421            Hello.world!\n",
            HexDump("Hello world!"));
  EXPECT_EQ(
      "0x0000:  5052 4920 2a20 4854 5450 2f32 2e30 0d0a  PRI.*.HTTP/2.0..\n"
      "0x0010:  0d0a 534d 0d0a 0d0a                      ..SM....\n",
      HexDump("PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"));
}

}  // namespace test

}  // namespace net
