// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/image_utils.h"

#include <stdint.h>

#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

namespace {

// EXPECT_EQ() macro doesn't work for templates that have multiple commas, e.g.:
//   EXPECT_EQ(0, GetSignedBits<0, 15>(int32_t(0)));
// would not compile. We get around this by wrapping with function.
template <typename T>
void expect_eq(const T& a, const T& b) {
  EXPECT_EQ(a, b);
}

// Casting functions to specify signed 8-bit and 16-bit integer constants.
inline int8_t signed8(const uint8_t& v) {
  return *reinterpret_cast<const int8_t*>(&v);
}

inline int32_t signed16(const uint16_t& v) {
  return *reinterpret_cast<const int16_t*>(&v);
}

}  // namespace

TEST(ImageUtilsTest, Bitness) {
  EXPECT_EQ(4U, WidthOf(kBit32));
  EXPECT_EQ(8U, WidthOf(kBit64));
}

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

TEST(ImageUtilsTest, GetBit) {
  // 0xC5 = 0b1100'0101.
  constexpr uint8_t v = 0xC5;
  expect_eq<uint8_t>(1, GetBit<0>(v));
  expect_eq<int8_t>(0, GetBit<1>(signed8(v)));
  expect_eq<uint8_t>(1, GetBit<2>(v));
  expect_eq<int8_t>(0, GetBit<3>(signed8(v)));
  expect_eq<uint8_t>(0, GetBit<4>(v));
  expect_eq<int8_t>(0, GetBit<5>(signed8(v)));
  expect_eq<uint8_t>(1, GetBit<6>(v));
  expect_eq<int8_t>(1, GetBit<7>(signed8(v)));

  expect_eq<int16_t>(1, GetBit<3, int16_t>(0x0008));
  expect_eq<uint16_t>(0, GetBit<14, uint16_t>(0xB000));
  expect_eq<uint16_t>(1, GetBit<15, uint16_t>(0xB000));

  expect_eq<uint32_t>(1, GetBit<0, uint32_t>(0xFFFFFFFF));
  expect_eq<uint32_t>(1, GetBit<31, int32_t>(0xFFFFFFFF));

  expect_eq<uint32_t>(0, GetBit<0, uint32_t>(0xFF00A596));
  expect_eq<int32_t>(1, GetBit<1, int32_t>(0xFF00A596));
  expect_eq<uint32_t>(1, GetBit<4, uint32_t>(0xFF00A596));
  expect_eq<int32_t>(1, GetBit<7, int32_t>(0xFF00A596));
  expect_eq<uint32_t>(0, GetBit<9, uint32_t>(0xFF00A596));
  expect_eq<int32_t>(0, GetBit<16, int32_t>(0xFF00A59));
  expect_eq<uint32_t>(1, GetBit<24, uint32_t>(0xFF00A596));
  expect_eq<int32_t>(1, GetBit<31, int32_t>(0xFF00A596));

  expect_eq<uint64_t>(0, GetBit<62, uint64_t>(0xB000000000000000ULL));
  expect_eq<int64_t>(1, GetBit<63, int64_t>(0xB000000000000000LL));
}

TEST(ImageUtilsTest, GetBits) {
  // Zero-extended: Basic cases for various values.
  uint32_t test_cases[] = {0, 1, 2, 7, 137, 0x10000, 0x69969669, 0xFFFFFFFF};
  for (uint32_t v : test_cases) {
    expect_eq<uint32_t>(v & 0xFF, GetUnsignedBits<0, 7>(v));
    expect_eq<uint32_t>((v >> 8) & 0xFF, GetUnsignedBits<8, 15>(v));
    expect_eq<uint32_t>((v >> 16) & 0xFF, GetUnsignedBits<16, 23>(v));
    expect_eq<uint32_t>((v >> 24) & 0xFF, GetUnsignedBits<24, 31>(v));
    expect_eq<uint32_t>(v & 0xFFFF, GetUnsignedBits<0, 15>(v));
    expect_eq<uint32_t>((v >> 1) & 0x3FFFFFFF, GetUnsignedBits<1, 30>(v));
    expect_eq<uint32_t>((v >> 2) & 0x0FFFFFFF, GetUnsignedBits<2, 29>(v));
    expect_eq<uint32_t>(v, GetUnsignedBits<0, 31>(v));
  }

  // Zero-extended: Reading off various nibbles.
  expect_eq<uint32_t>(0x4, GetUnsignedBits<20, 23>(0x00432100U));
  expect_eq<uint32_t>(0x43, GetUnsignedBits<16, 23>(0x00432100));
  expect_eq<uint32_t>(0x432, GetUnsignedBits<12, 23>(0x00432100U));
  expect_eq<uint32_t>(0x4321, GetUnsignedBits<8, 23>(0x00432100));
  expect_eq<uint32_t>(0x321, GetUnsignedBits<8, 19>(0x00432100U));
  expect_eq<uint32_t>(0x21, GetUnsignedBits<8, 15>(0x00432100));
  expect_eq<uint32_t>(0x1, GetUnsignedBits<8, 11>(0x00432100U));

  // Sign-extended: 0x3CA5 = 0b0011'1100'1010'0101.
  expect_eq<int16_t>(signed16(0xFFFF), GetSignedBits<0, 0>(0x3CA5U));
  expect_eq<int16_t>(signed16(0x0001), GetSignedBits<0, 1>(0x3CA5));
  expect_eq<int16_t>(signed16(0xFFFD), GetSignedBits<0, 2>(0x3CA5U));
  expect_eq<int16_t>(signed16(0x0005), GetSignedBits<0, 4>(0x3CA5));
  expect_eq<int16_t>(signed16(0xFFA5), GetSignedBits<0, 7>(0x3CA5U));
  expect_eq<int16_t>(signed16(0xFCA5), GetSignedBits<0, 11>(0x3CA5));
  expect_eq<int16_t>(signed16(0x0005), GetSignedBits<0, 3>(0x3CA5U));
  expect_eq<int16_t>(signed16(0xFFFA), GetSignedBits<4, 7>(0x3CA5));
  expect_eq<int16_t>(signed16(0xFFFC), GetSignedBits<8, 11>(0x3CA5U));
  expect_eq<int16_t>(signed16(0x0003), GetSignedBits<12, 15>(0x3CA5));
  expect_eq<int16_t>(signed16(0x0000), GetSignedBits<4, 4>(0x3CA5U));
  expect_eq<int16_t>(signed16(0xFFFF), GetSignedBits<5, 5>(0x3CA5));
  expect_eq<int16_t>(signed16(0x0002), GetSignedBits<4, 6>(0x3CA5U));
  expect_eq<int16_t>(signed16(0x1E52), GetSignedBits<1, 14>(0x3CA5));
  expect_eq<int16_t>(signed16(0xFF29), GetSignedBits<2, 13>(0x3CA5U));
  expect_eq<int32_t>(0x00001E52, GetSignedBits<1, 14>(0x3CA5));
  expect_eq<int32_t>(0xFFFFFF29, GetSignedBits<2, 13>(0x3CA5U));

  // 64-bits: Extract from middle 0x66 = 0b0110'0110.
  expect_eq<uint64_t>(0x0000000000000009LL,
                      GetUnsignedBits<30, 33>(int64_t(0x2222222661111111LL)));
  expect_eq<int64_t>(0xFFFFFFFFFFFFFFF9LL,
                     GetSignedBits<30, 33>(uint64_t(0x2222222661111111LL)));
}

TEST(ImageUtilsTest, SignExtend) {
  // 0x6A = 0b0110'1010.
  expect_eq<uint8_t>(0x00, SignExtend<0, uint8_t>(0x6A));
  expect_eq<int8_t>(signed8(0xFE), SignExtend<1, int8_t>(signed8(0x6A)));
  expect_eq<uint8_t>(0x02, SignExtend<2, uint8_t>(0x6A));
  expect_eq<int8_t>(signed8(0xFA), SignExtend<3, int8_t>(signed8(0x6A)));
  expect_eq<uint8_t>(0x0A, SignExtend<4, uint8_t>(0x6A));
  expect_eq<int8_t>(signed8(0xEA), SignExtend<5, int8_t>(signed8(0x6A)));
  expect_eq<uint8_t>(0xEA, SignExtend<6, uint8_t>(0x6A));
  expect_eq<int8_t>(signed8(0x6A), SignExtend<7, int8_t>(signed8(0x6A)));

  expect_eq<int16_t>(signed16(0xFFFA), SignExtend<3, int16_t>(0x6A));
  expect_eq<uint16_t>(0x000A, SignExtend<4, uint16_t>(0x6A));

  expect_eq<int32_t>(0xFFFF8000, SignExtend<15, int32_t>(0x00008000));
  expect_eq<uint32_t>(0x00008000U, SignExtend<16, uint32_t>(0x00008000));
  expect_eq<int32_t>(0xFFFFFC00, SignExtend<10, int32_t>(0x00000400));
  expect_eq<uint32_t>(0xFFFFFFFFU, SignExtend<31, uint32_t>(0xFFFFFFFF));

  expect_eq<int64_t>(0xFFFFFFFFFFFFFE6ALL,
                     SignExtend<9, int64_t>(0x000000000000026ALL));
  expect_eq<int64_t>(0x000000000000016ALL,
                     SignExtend<9, int64_t>(0xFFFFFFFFFFFFFD6ALL));
  expect_eq<uint64_t>(0xFFFFFFFFFFFFFE6AULL,
                      SignExtend<9, uint64_t>(0x000000000000026AULL));
  expect_eq<uint64_t>(0x000000000000016AULL,
                      SignExtend<9, uint64_t>(0xFFFFFFFFFFFFFD6AULL));
}

TEST(ImageUtilsTest, SignedFit) {
  for (int v = -0x80; v < 0x80; ++v) {
    expect_eq<bool>(v >= -1 && v < 1, SignedFit<1, int8_t>(v));
    expect_eq<bool>(v >= -1 && v < 1, SignedFit<1, uint8_t>(v));
    expect_eq<bool>(v >= -2 && v < 2, SignedFit<2, int8_t>(v));
    expect_eq<bool>(v >= -4 && v < 4, SignedFit<3, uint8_t>(v));
    expect_eq<bool>(v >= -8 && v < 8, SignedFit<4, int16_t>(v));
    expect_eq<bool>(v >= -16 && v < 16, SignedFit<5, uint32_t>(v));
    expect_eq<bool>(v >= -32 && v < 32, SignedFit<6, int32_t>(v));
    expect_eq<bool>(v >= -64 && v < 64, SignedFit<7, uint64_t>(v));
    expect_eq<bool>(true, SignedFit<8, int8_t>(v));
    expect_eq<bool>(true, SignedFit<8, uint8_t>(v));
  }

  expect_eq<bool>(true, SignedFit<16, uint32_t>(0x00000000));
  expect_eq<bool>(true, SignedFit<16, uint32_t>(0x00007FFF));
  expect_eq<bool>(true, SignedFit<16, uint32_t>(0xFFFF8000));
  expect_eq<bool>(true, SignedFit<16, uint32_t>(0xFFFFFFFF));
  expect_eq<bool>(true, SignedFit<16, int32_t>(0x00007FFF));
  expect_eq<bool>(true, SignedFit<16, int32_t>(0xFFFF8000));

  expect_eq<bool>(false, SignedFit<16, uint32_t>(0x80000000));
  expect_eq<bool>(false, SignedFit<16, uint32_t>(0x7FFFFFFF));
  expect_eq<bool>(false, SignedFit<16, uint32_t>(0x00008000));
  expect_eq<bool>(false, SignedFit<16, uint32_t>(0xFFFF7FFF));
  expect_eq<bool>(false, SignedFit<16, int32_t>(0x00008000));
  expect_eq<bool>(false, SignedFit<16, int32_t>(0xFFFF7FFF));

  expect_eq<bool>(true, SignedFit<48, int64_t>(0x00007FFFFFFFFFFFLL));
  expect_eq<bool>(true, SignedFit<48, int64_t>(0xFFFF800000000000LL));
  expect_eq<bool>(false, SignedFit<48, int64_t>(0x0008000000000000LL));
  expect_eq<bool>(false, SignedFit<48, int64_t>(0xFFFF7FFFFFFFFFFFLL));
}

}  // namespace zucchini
