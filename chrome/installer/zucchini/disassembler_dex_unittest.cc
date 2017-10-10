// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/disassembler_dex.h"

#include <stdint.h>

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

TEST(DisassemblerDexTest, DecodeUleb128) {
  std::vector<uint8_t> data = {0x7F};
  uint32_t result = 0;
  dex::DecodeUleb128(data.begin(), &result);
  EXPECT_EQ(0x7FU, result);
}

TEST(DisassemblerDexTest, DecodeSleb128) {
  std::vector<uint8_t> data = {0x7F};
  int32_t result = 0;
  dex::DecodeSleb128(data.begin(), &result);
  EXPECT_EQ(-1, result);
}

}  // namespace zucchini
