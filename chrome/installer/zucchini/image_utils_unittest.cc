// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/image_utils.h"

#include <stdint.h>

#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

TEST(ImageUtilsTest, IsMarked) {
  EXPECT_FALSE(IsMarked(0x00000000));
  EXPECT_TRUE(IsMarked(0x80000000));

  EXPECT_FALSE(IsMarked(0x00000001));
  EXPECT_TRUE(IsMarked(0x80000001));

  EXPECT_FALSE(IsMarked(0x70000000));
  EXPECT_TRUE(IsMarked(0xF0000000));

  EXPECT_FALSE(IsMarked(0x7FFFFFFF));
  EXPECT_TRUE(IsMarked(0xFFFFFFFF));

  EXPECT_FALSE(IsMarked(0x70000000));
  EXPECT_TRUE(IsMarked(0xC0000000));

  EXPECT_FALSE(IsMarked(0x0000BEEF));
  EXPECT_TRUE(IsMarked(0x8000BEEF));
}

TEST(ImageUtilsTest, MarkIndex) {
  EXPECT_EQ(offset_t(0x80000000), MarkIndex(0x00000000));
  EXPECT_EQ(offset_t(0x80000000), MarkIndex(0x80000000));

  EXPECT_EQ(offset_t(0x80000001), MarkIndex(0x00000001));
  EXPECT_EQ(offset_t(0x80000001), MarkIndex(0x80000001));

  EXPECT_EQ(offset_t(0xF0000000), MarkIndex(0x70000000));
  EXPECT_EQ(offset_t(0xF0000000), MarkIndex(0xF0000000));

  EXPECT_EQ(offset_t(0xFFFFFFFF), MarkIndex(0x7FFFFFFF));
  EXPECT_EQ(offset_t(0xFFFFFFFF), MarkIndex(0xFFFFFFFF));

  EXPECT_EQ(offset_t(0xC0000000), MarkIndex(0x40000000));
  EXPECT_EQ(offset_t(0xC0000000), MarkIndex(0xC0000000));

  EXPECT_EQ(offset_t(0x8000BEEF), MarkIndex(0x0000BEEF));
  EXPECT_EQ(offset_t(0x8000BEEF), MarkIndex(0x8000BEEF));
}

TEST(ImageUtilsTest, UnmarkIndex) {
  EXPECT_EQ(offset_t(0x00000000), UnmarkIndex(0x00000000));
  EXPECT_EQ(offset_t(0x00000000), UnmarkIndex(0x80000000));

  EXPECT_EQ(offset_t(0x00000001), UnmarkIndex(0x00000001));
  EXPECT_EQ(offset_t(0x00000001), UnmarkIndex(0x80000001));

  EXPECT_EQ(offset_t(0x70000000), UnmarkIndex(0x70000000));
  EXPECT_EQ(offset_t(0x70000000), UnmarkIndex(0xF0000000));

  EXPECT_EQ(offset_t(0x7FFFFFFF), UnmarkIndex(0x7FFFFFFF));
  EXPECT_EQ(offset_t(0x7FFFFFFF), UnmarkIndex(0xFFFFFFFF));

  EXPECT_EQ(offset_t(0x40000000), UnmarkIndex(0x40000000));
  EXPECT_EQ(offset_t(0x40000000), UnmarkIndex(0xC0000000));

  EXPECT_EQ(offset_t(0x0000BEEF), UnmarkIndex(0x0000BEEF));
  EXPECT_EQ(offset_t(0x0000BEEF), UnmarkIndex(0x8000BEEF));
}

TEST(ImageUtilsTest, RangeIsBounded) {
  // Basic tests.
  EXPECT_TRUE(RangeIsBounded<uint8_t>(0U, 0U, 10U));
  EXPECT_TRUE(RangeIsBounded<uint8_t>(0U, 9U, 10U));
  EXPECT_FALSE(RangeIsBounded<uint8_t>(1U, 9U, 10U));
  EXPECT_TRUE(RangeIsBounded<uint8_t>(8U, 1U, 10U));
  EXPECT_TRUE(RangeIsBounded<uint8_t>(8U, 2U, 10U));
  EXPECT_TRUE(RangeIsBounded<uint8_t>(9U, 0U, 10U));
  EXPECT_FALSE(RangeIsBounded<uint8_t>(100U, 0U, 10U));
  EXPECT_FALSE(RangeIsBounded<uint8_t>(100U, 1U, 10U));

  // Test at boundary of overflow.
  EXPECT_TRUE(RangeIsBounded<uint8_t>(42U, 137U, 255U));
  EXPECT_TRUE(RangeIsBounded<uint8_t>(0U, 254U, 255U));
  EXPECT_FALSE(RangeIsBounded<uint8_t>(0U, 255U, 255U));
  EXPECT_FALSE(RangeIsBounded<uint8_t>(1U, 254U, 255U));
  EXPECT_TRUE(RangeIsBounded<uint8_t>(254U, 0U, 255U));
  EXPECT_FALSE(RangeIsBounded<uint8_t>(254U, 1U, 255U));
  EXPECT_FALSE(RangeIsBounded<uint8_t>(255U, 0U, 255U));
  EXPECT_FALSE(RangeIsBounded<uint8_t>(255U, 3U, 255U));
}

}  // namespace zucchini
