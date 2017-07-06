// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/buffer_source.h"

#include <cstdint>
#include <iterator>

#include "base/test/gtest_util.h"

namespace zucchini {

static constexpr size_t kLen = 10;
constexpr const uint8_t bytes[kLen] = {0x10, 0x32, 0x54, 0x76, 0x98,
                                       0xBA, 0xDC, 0xFE, 0x10, 0x00};

class BufferSourceTest : public testing::Test {
 protected:
  BufferSource source_ = {std::begin(bytes), kLen};
};

TEST_F(BufferSourceTest, ExpectValue) {
  EXPECT_TRUE(source_.ExpectValue(uint32_t(0x76543210)));
  EXPECT_FALSE(source_.ExpectValue(uint32_t(0x0)));
  EXPECT_TRUE(source_.ExpectValue(uint64_t(0xFEDCBA9876543210)));
  EXPECT_FALSE(source_.ExpectValue(uint64_t(0x0)));
}

TEST_F(BufferSourceTest, ReadValue) {
  uint32_t value = 0;
  EXPECT_TRUE(source_.ReadValue(&value));
  EXPECT_EQ(uint32_t(0x76543210), value);
  EXPECT_EQ(size_t(6), source_.Remaining());

  EXPECT_TRUE(source_.ReadValue(&value));
  EXPECT_EQ(uint32_t(0xFEDCBA98), value);
  EXPECT_EQ(size_t(2), source_.Remaining());

  EXPECT_FALSE(source_.ReadValue(&value));
}

TEST_F(BufferSourceTest, GetRegion) {
  BufferView region = source_.GetRegion(0);
  EXPECT_EQ(std::begin(bytes), source_.begin());
  EXPECT_EQ(std::begin(bytes), region.begin());
  EXPECT_EQ(kLen, source_.Remaining());
  EXPECT_EQ(size_t(0), region.size());

  region = source_.GetRegion(2);
  EXPECT_EQ(std::begin(bytes), region.begin());
  EXPECT_EQ(size_t(2), region.size());
  EXPECT_EQ(region.end(), source_.begin());
  EXPECT_EQ(size_t(8), source_.Remaining());

  region = source_.GetRegion(kLen);
  EXPECT_EQ(source_.begin(), source_.end());
  EXPECT_EQ(std::end(bytes), source_.begin());
  EXPECT_EQ(size_t(8), region.size());
  EXPECT_EQ(region.end(), source_.begin());
  EXPECT_EQ(size_t(0), source_.Remaining());
}

}  // namespace zucchini
