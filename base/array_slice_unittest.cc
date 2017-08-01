// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/array_slice.h"

#include <array>
#include <initializer_list>
#include <vector>

#include "base/memory/ptr_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {

TEST(ArraySliceTest, DefaultConstructor) {
  ArraySlice<int> slice;
  EXPECT_EQ(nullptr, slice.data());
  EXPECT_EQ(0, slice.size());
}

TEST(ArraySliceTest, NullPtrConstructor) {
  ArraySlice<int> slice(nullptr);
  EXPECT_EQ(nullptr, slice.data());
  EXPECT_EQ(0, slice.size());
}

TEST(ArraySliceTest, DynamicArrayConstructor) {
  auto dynamic_array = base::MakeUnique<int[]>(10);
  ArraySlice<int> slice(dynamic_array.get(), 10);
  EXPECT_EQ(dynamic_array.get(), slice.data());
  EXPECT_EQ(10, slice.size());
}

TEST(ArraySliceTest, StaticArrayConstructor) {
  int static_array[10];
  ArraySlice<int> slice(static_array);
  EXPECT_EQ(static_array, slice.data());
  EXPECT_EQ(10, slice.size());
}

TEST(ArraySliceTest, ContainerConstructor) {
  {
    std::array<int, 10> arr;
    ArraySlice<int> slice = arr;
    EXPECT_EQ(arr.data(), slice.data());
    EXPECT_EQ(10, slice.size());
  }

  {
    std::vector<int> vec(10);
    ArraySlice<int> slice = vec;
    EXPECT_EQ(vec.data(), slice.data());
    EXPECT_EQ(10, slice.size());
  }
}

TEST(ArraySliceTest, InitializerListConstructor) {
  std::initializer_list<int> ilist = {1, 2, 3, 4, 5, 6};
  ArraySlice<int> slice = ilist;
  EXPECT_EQ(ilist.begin(), slice.data());
  EXPECT_EQ(6, slice.size());
}

TEST(ArraySliceTest, First) {
  ArraySlice<int> slice = {1, 2, 3, 4, 5, 6};
  ArraySlice<int> sub_slice = {1, 2, 3};
  EXPECT_EQ(sub_slice, slice.first(3));
}

TEST(ArraySliceTest, Last) {
  ArraySlice<int> slice = {1, 2, 3, 4, 5, 6};
  ArraySlice<int> sub_slice = {4, 5, 6};
  EXPECT_EQ(sub_slice, slice.last(3));
}

TEST(ArraySliceTest, SubSlice) {
  ArraySlice<int> slice = {1, 2, 3, 4, 5, 6};
  ArraySlice<int> sub_slice = {2, 3, 4, 5};
  ArraySlice<int> suffix = {5, 6};
  EXPECT_EQ(sub_slice, slice.subslice(1, 4));
  EXPECT_EQ(suffix, slice.subslice(5));
}

TEST(ArraySliceTest, Length) {
  ArraySlice<int> slice = {1, 2, 3, 4, 5, 6};
  EXPECT_EQ(6, slice.length());
}

TEST(ArraySliceTest, Size) {
  ArraySlice<int> slice = {1, 2, 3, 4, 5, 6};
  EXPECT_EQ(6, slice.size());
}

TEST(ArraySliceTest, Empty) {
  ArraySlice<int> slice = {1, 2, 3, 4, 5, 6};
  EXPECT_FALSE(slice.empty());
  EXPECT_TRUE(ArraySlice<char>().empty());
}

TEST(ArraySliceTest, OperatorAt) {
  ArraySlice<int> slice = {0, 1, 2};
  EXPECT_EQ(0, slice[0]);
  EXPECT_EQ(1, slice[1]);
  EXPECT_EQ(2, slice[2]);
}

TEST(ArraySliceTest, Front) {
  ArraySlice<int> slice = {0, 1, 2};
  EXPECT_EQ(0, slice.front());
}

TEST(ArraySliceTest, Back) {
  ArraySlice<int> slice = {0, 1, 2};
  EXPECT_EQ(2, slice.back());
}

TEST(ArraySliceTest, BeginEnd) {
  std::vector<int> vec = {0, 1, 2};
  ArraySlice<int> slice = vec;
  EXPECT_EQ(3, std::distance(slice.begin(), slice.end()));
  EXPECT_TRUE(std::equal(slice.begin(), slice.end(), vec.begin()));
}

TEST(ArraySliceTest, CBeginCEnd) {
  std::vector<int> vec = {0, 1, 2};
  ArraySlice<int> slice = vec;
  EXPECT_EQ(3, std::distance(slice.cbegin(), slice.cend()));
  EXPECT_TRUE(std::equal(slice.cbegin(), slice.cend(), vec.cbegin()));
}

TEST(ArraySliceTest, RBeginREnd) {
  std::vector<int> vec = {0, 1, 2};
  ArraySlice<int> slice = vec;
  EXPECT_EQ(3, std::distance(slice.rbegin(), slice.rend()));
  EXPECT_TRUE(std::equal(slice.rbegin(), slice.rend(), vec.rbegin()));
}

TEST(ArraySliceTest, CRBeginCREnd) {
  std::vector<int> vec = {0, 1, 2};
  ArraySlice<int> slice = vec;
  EXPECT_EQ(3, std::distance(slice.crbegin(), slice.crend()));
  EXPECT_TRUE(std::equal(slice.crbegin(), slice.crend(), vec.crbegin()));
}

TEST(ArraySliceTest, OperatorEQ) {
  std::vector<int> vec3 = {0, 1, 2};
  EXPECT_EQ(ArraySlice<int>(vec3), ArraySlice<int>(vec3));
}

TEST(ArraySliceTest, OperatorNE) {
  std::vector<int> vec3 = {0, 1, 2};
  std::vector<int> vec4 = {0, 1, 2, 3};
  EXPECT_NE(ArraySlice<int>(vec3), ArraySlice<int>(vec4));
}

TEST(ArraySliceTest, OperatorLT) {
  std::vector<int> vec3 = {0, 1, 2};
  std::vector<int> vec4 = {0, 1, 2, 3};
  EXPECT_LT(ArraySlice<int>(vec3), ArraySlice<int>(vec4));
}

TEST(ArraySliceTest, OperatorGT) {
  std::vector<int> vec3 = {0, 1, 2};
  std::vector<int> vec4 = {0, 1, 2, 3};
  EXPECT_GT(ArraySlice<int>(vec4), ArraySlice<int>(vec3));
}

TEST(ArraySliceTest, OperatorLE) {
  std::vector<int> vec3 = {0, 1, 2};
  std::vector<int> vec4 = {0, 1, 2, 3};
  EXPECT_LE(ArraySlice<int>(vec3), ArraySlice<int>(vec3));
  EXPECT_LE(ArraySlice<int>(vec3), ArraySlice<int>(vec4));
}

TEST(ArraySliceTest, OperatorGE) {
  std::vector<int> vec3 = {0, 1, 2};
  std::vector<int> vec4 = {0, 1, 2, 3};
  EXPECT_GE(ArraySlice<int>(vec3), ArraySlice<int>(vec3));
  EXPECT_GE(ArraySlice<int>(vec4), ArraySlice<int>(vec3));
}

}  // namespace base
