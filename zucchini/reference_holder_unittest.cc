// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/reference_holder.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "zucchini/image_utils.h"

namespace zucchini {

class ReferenceHolderTest : public testing::Test {
 public:
  ReferenceHolderTest() : refs_(3) {}

  ReferenceHolder refs_;

  std::vector<Reference> type0_sorted = {{3, 7}, {5, 5}, {6, 9}};
  std::vector<Reference> type1_sorted = {{2, 6}, {4, 5}};
  std::vector<Reference> type2_sorted = {{4, 8}, {5, 2}, {7, 9}};
  std::vector<TypedReference> all_sorted_by_type = {
      {0, 3, 7}, {0, 5, 5}, {0, 6, 9}, {1, 2, 6},
      {1, 4, 5}, {2, 4, 8}, {2, 5, 2}, {2, 7, 9}};
  std::vector<TypedReference> all_sorted_by_location = {
      {1, 2, 6}, {0, 3, 7}, {1, 4, 5}, {2, 4, 8},
      {0, 5, 5}, {2, 5, 2}, {0, 6, 9}, {2, 7, 9}};

  void FillSimple() {
    refs_.Insert({1, 0, 0}, type0_sorted);
    refs_.Insert({1, 1, 1}, type1_sorted);
    refs_.Insert({1, 2, 2}, type2_sorted);
  }
};

TEST_F(ReferenceHolderTest, AccessByType) {
  FillSimple();
  EXPECT_TRUE(ranges::equal(type0_sorted, refs_.Get(0)));
  EXPECT_TRUE(ranges::equal(type1_sorted, refs_.Get(1)));
  EXPECT_TRUE(ranges::equal(type2_sorted, refs_.Get(2)));
}

TEST_F(ReferenceHolderTest, AccessSortedByType) {
  FillSimple();
  EXPECT_TRUE(ranges::equal(all_sorted_by_type, refs_.Get(SortedByType)));
}

TEST_F(ReferenceHolderTest, AccessSortedByLocation) {
  FillSimple();
  EXPECT_TRUE(
      ranges::equal(all_sorted_by_location, refs_.Get(SortedByLocation)));
}

}  // namespace zucchini
