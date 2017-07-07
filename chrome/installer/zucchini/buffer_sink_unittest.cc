// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/buffer_sink.h"

#include <vector>

#include "base/test/gtest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

class BufferSinkTest : public testing::Test {
 protected:
  void SetUp() override { sink_ = BufferSink(buffer_.data(), buffer_.size()); }

  std::vector<uint8_t> buffer_ = std::vector<uint8_t>(10, 0);
  BufferSink sink_;
};

TEST_F(BufferSinkTest, PutValue) {
  EXPECT_TRUE(sink_.PutValue(uint32_t(0x76543210)));
  EXPECT_TRUE(sink_.PutValue(uint32_t(0xFEDCBA98)));
  EXPECT_FALSE(sink_.PutValue(uint32_t(0x00)));
  EXPECT_TRUE(sink_.PutValue(uint16_t(0x0010)));
  EXPECT_EQ(std::vector<uint8_t>(
                {0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE, 0x10, 0x00}),
            buffer_);
}

TEST_F(BufferSinkTest, PutRange) {
  std::vector<uint8_t> range = {0x10, 0x32, 0x54, 0x76, 0x98, 0xBA,
                                0xDC, 0xFE, 0x10, 0x00, 0x42};
  EXPECT_FALSE(sink_.PutRange(range.begin(), range.end()));
  EXPECT_TRUE(sink_.PutRange(range.begin(), range.begin() + 10));
}

}  // namespace zucchini
