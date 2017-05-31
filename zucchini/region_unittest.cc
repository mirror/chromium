// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/region.h"

#include <cstdint>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

TEST(RegionTest, Crc32) {
  std::vector<uint8_t> bytes = {0x10, 0x32, 0x54, 0x76, 0x98,
                                0xBA, 0xDC, 0xFE, 0x10, 0x00};

  // Empty region.
  EXPECT_EQ(0x00000000U, Region(&bytes[0], static_cast<size_t>(0)).crc32());

  // Single byte.
  EXPECT_EQ(0xCFB5FFE9U, Region(&bytes[0], 1).crc32());

  // Same byte (0x10) appearing at different location.
  EXPECT_EQ(0xCFB5FFE9U, Region(&bytes[8], 1).crc32());

  // Single byte of 0.
  EXPECT_EQ(0xD202EF8DU, Region(&bytes[9], 1).crc32());

  // Whole region.
  EXPECT_EQ(0xA86FD7D6U, Region(&bytes[0], 10).crc32());

  // Whole region excluding 0 at end.
  EXPECT_EQ(0x0762F38BU, Region(&bytes[0], 9).crc32());
}

}  // namespace zucchini
