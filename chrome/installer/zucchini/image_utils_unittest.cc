// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/image_utils.h"

#include <cstdint>

#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

TEST(ImageUtilsTest, IsMarked) {
  EXPECT_FALSE(IsMarked(0x00000000));
  EXPECT_TRUE(IsMarked(0x80000000));
  EXPECT_FALSE(IsMarked(0x77777777));
  EXPECT_TRUE(IsMarked(0x80000001));
  EXPECT_FALSE(IsMarked(0x70000000));
  EXPECT_TRUE(IsMarked(0xC0000000));
}

TEST(ImageUtilsTest, MarkIndex) {
  EXPECT_EQ(0x80000000, MarkIndex(0x00000000));
  EXPECT_EQ(0x80000000, MarkIndex(0x80000000));
  EXPECT_EQ(0xF7777777, MarkIndex(0x77777777));
  EXPECT_EQ(0x80000001, MarkIndex(0x80000001));
  EXPECT_EQ(0xC0000000, MarkIndex(0x40000000));
  EXPECT_EQ(0xC0000000, MarkIndex(0xC0000000));
}

TEST(ImageUtilsTest, UnmarkIndex) {
  EXPECT_EQ(0x00000000, UnmarkIndex(0x00000000));
  EXPECT_EQ(0x00000000, UnmarkIndex(0x80000000));
  EXPECT_EQ(0x77777777, UnmarkIndex(0x77777777));
  EXPECT_EQ(0x00000001, UnmarkIndex(0x80000001));
  EXPECT_EQ(0x40000000, UnmarkIndex(0x40000000));
  EXPECT_EQ(0x40000000, UnmarkIndex(0xC0000000));
}

}  // namespace zucchini
