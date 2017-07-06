// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/patch_source.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

using vec = std::vector<uint8_t>;

template <class T>
T TestParse(const std::vector<uint8_t>& buffer) {
  T value;
  BufferSource buffer_source(buffer.data(), buffer.size());
  EXPECT_TRUE(Parse(&value, &buffer_source));
  EXPECT_TRUE(buffer_source.empty());
  return value;
}

template <class T>
void TestInvalidParse(const std::vector<uint8_t>& buffer) {
  T value;
  BufferSource buffer_source(buffer.data(), buffer.size());
  EXPECT_FALSE(Parse(&value, &buffer_source));
}

template <class T>
void TestParseMember(const std::vector<uint8_t>& buffer, T* value) {
  BufferSource buffer_source(buffer.data(), buffer.size());
  EXPECT_TRUE(value->Parse(&buffer_source));
  EXPECT_TRUE(buffer_source.empty());
}

TEST(Parse, ElementMatch) {
  ElementMatch element_match = TestParse<ElementMatch>({
      1, 0, 0, 0,              // old_offset
      3, 0, 0, 0,              // new_offset
      2, 0, 0, 0, 0, 0, 0, 0,  // old_length
      4, 0, 0, 0, 0, 0, 0, 0,  // new_length
      6, 0, 0, 0,              // EXE_TYPE_DEX
  });

  EXPECT_EQ(EXE_TYPE_DEX, element_match.old_element.exe_type);
  EXPECT_EQ(EXE_TYPE_DEX, element_match.new_element.exe_type);
}

TEST(Parse, ElementMatchTooSmall) {
  TestInvalidParse<ElementMatch>({0});
}

TEST(Serialize, Buffer) {
  BufferSource buffer = TestParse<BufferSource>({0, 0, 0, 0, 0, 0, 0, 0});
  buffer = TestParse<BufferSource>({3, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3});
  TestInvalidParse<BufferSource>({3, 0, 0, 0, 0, 0, 0, 0, 1, 2});
}

TEST(EquivalenceSourceTest, Serialize) {
  EquivalenceSource equivalence_source;
  TestParseMember(
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      &equivalence_source);

  EXPECT_FALSE(equivalence_source.GetNext());
  EXPECT_TRUE(equivalence_source.Done());
}

}  // namespace zucchini
