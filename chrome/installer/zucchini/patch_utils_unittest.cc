// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/patch_utils.h"

#include "base/test/gtest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

TEST(PatchUtilsTest, EncodeDecodeVarInt32) {
  constexpr uint32_t data[] = {0,       64,      128,        8192,
                               16384,   1 << 20, 1 << 21,    1 << 22,
                               1 << 27, 1 << 28, 0x7FFFFFFF, UINT32_MAX};
  std::vector<uint8_t> buffer;

  std::vector<uint32_t> values;
  for (size_t i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
    uint32_t basis = data[i];
    for (int delta = -4; delta <= 4; ++delta) {
      int32_t value = basis + delta;
      EncodeVarUInt<uint32_t>(value, std::back_inserter(buffer));
      values.push_back(value);

      value = delta - basis;
      EncodeVarUInt<uint32_t>(value, std::back_inserter(buffer));
      values.push_back(value);
    }
  }

  auto it = buffer.begin();
  for (size_t i = 0; i < values.size(); ++i) {
    uint32_t value;
    auto res = DecodeVarUInt(it, buffer.end(), &value);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(values[i], value);
    it = res.value();
  }
  EXPECT_EQ(it, buffer.end());

  uint32_t dummy;
  auto res = DecodeVarUInt(it, buffer.end(), &dummy);
  EXPECT_FALSE(res.has_value());
}

TEST(PatchUtilsTest, EncodeDecodeVarUInt32Signed) {
  constexpr const int32_t data[] = {
      0,       64,      128,     8192, 16384,     1 << 20,  1 << 21,
      1 << 22, 1 << 27, 1 << 28, -1,   INT32_MIN, INT32_MAX};
  std::vector<uint8_t> buffer;

  std::vector<int32_t> values;
  for (size_t i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
    int32_t basis = data[i];
    for (int delta = -4; delta <= 4; ++delta) {
      int32_t value = basis + delta;
      EncodeVarInt(value, std::back_inserter(buffer));
      values.push_back(basis + delta);

      value = -basis + delta;
      EncodeVarInt(value, std::back_inserter(buffer));
      values.push_back(value);
    }
  }

  auto it = buffer.begin();
  for (size_t i = 0; i < values.size(); ++i) {
    int32_t value;
    auto res = DecodeVarInt(it, buffer.end(), &value);
    EXPECT_TRUE(res.has_value());
    EXPECT_EQ(values[i], value);
    it = res.value();
  }
  int32_t dummy;
  auto res = DecodeVarInt(it, buffer.end(), &dummy);
  EXPECT_FALSE(res.has_value());
}

}  // namespace zucchini
