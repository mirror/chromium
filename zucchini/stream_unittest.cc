// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/stream.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

TEST(StreamsTest, SimpleWriteRead) {
  std::vector<uint8_t> buffer;
  auto sink = MakeSinkStream(std::back_inserter(buffer));

  const uint8_t data[] = "Hello";
  for (size_t i = 0; i < 5; ++i) {
    sink(data[i]);
  }

  auto source = MakeSourceStream(buffer);

  const uint8_t* it = data;
  uint8_t value;
  while (!source(&value)) {
    EXPECT_EQ(*(it++), value);
  }
}

TEST(StreamsTest, WriteReadVarint32) {
  static const uint32_t data[] = {0,       64,      128,        8192,
                                  16384,   1 << 20, 1 << 21,    1 << 22,
                                  1 << 27, 1 << 28, 0x7FFFFFFF, UINT32_MAX};
  std::vector<uint8_t> buffer;
  auto sink = MakeSinkStream(std::back_inserter(buffer));

  std::vector<uint32_t> values;
  for (size_t i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
    uint32_t basis = data[i];
    for (int delta = -4; delta <= 4; ++delta) {
      sink(basis + delta);
      values.push_back(basis + delta);
      sink(delta - basis);
      values.push_back(delta - basis);
    }
  }

  auto source = MakeSourceStream(buffer);

  for (size_t i = 0; i < values.size(); ++i) {
    uint32_t value;
    EXPECT_TRUE(source(&value));
    EXPECT_EQ(values[i], value);
  }
  uint32_t dummy;
  EXPECT_FALSE(source(&dummy));
}

TEST(StreamsTest, WriteReadVarint32Signed) {
  static const int32_t data[] = {0,       64,        128,      8192,    16384,
                                 1 << 20, 1 << 21,   1 << 22,  1 << 27, 1 << 28,
                                 -1,      INT32_MIN, INT32_MAX};
  std::vector<uint8_t> buffer;
  auto sink = MakeSinkStream(std::back_inserter(buffer));

  std::vector<int32_t> values;
  for (size_t i = 0; i < sizeof(data) / sizeof(data[0]); ++i) {
    int32_t basis = data[i];
    for (int delta = -4; delta <= 4; ++delta) {
      sink(basis + delta);
      values.push_back(basis + delta);
      sink(-basis + delta);
      values.push_back(-basis + delta);
    }
  }

  auto source = MakeSourceStream(buffer);

  for (size_t i = 0; i < values.size(); ++i) {
    int32_t value;
    EXPECT_TRUE(source(&value));
    EXPECT_EQ(values[i], value);
  }
  int32_t dummy;
  EXPECT_FALSE(source(&dummy));
}

}  // namespace zucchini
