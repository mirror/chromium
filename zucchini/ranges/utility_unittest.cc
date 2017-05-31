// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/ranges/utility.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "zucchini/ranges/range_facade.h"
#include "zucchini/ranges/view.h"

namespace zucchini {
namespace ranges {

class RangesUtilityTest : public testing::Test {
 public:
  using WrappedVectorInt = ranges::View<std::vector<int>>;
  using WrappedVectorIntConst = ranges::View<const std::vector<int>&>;

  RangesUtilityTest() {
    list1 = {3, 1, 4, 1, 5, 9, 2};
    list1_longer = {3, 1, 4, 1, 5, 9, 2, 6};
    list2 = {2, 1, 7, 1, 8, 2, 8};
  }

 protected:
  static void HelperEqual(std::vector<int>* list1,
                          std::vector<int>* list2,
                          bool exp) {
    // size() supported.
    EXPECT_EQ(exp, ranges::equal(*list1, *list2));

    // Using clone of list2.
    std::vector<int> list2_clone;
    for (int v : *list2)
      list2_clone.push_back(v);
    EXPECT_EQ(exp, ranges::equal(*list1, list2_clone));

    // Using const reference.
    const std::vector<int>& list2_const = list2_clone;
    EXPECT_EQ(exp, ranges::equal(*list1, list2_const));
    EXPECT_EQ(exp, ranges::equal(list2_const, *list1));

    // size() unsupported.
    EXPECT_EQ(exp, ranges::equal(*list1, WrappedVectorInt{*list2}));
    EXPECT_EQ(exp, ranges::equal(WrappedVectorInt{*list1}, *list2));
    EXPECT_EQ(
        exp, ranges::equal(WrappedVectorInt{*list1}, WrappedVectorInt{*list2}));
    EXPECT_EQ(exp, ranges::equal(WrappedVectorInt{*list1}, list2_const));
    EXPECT_EQ(exp, ranges::equal(WrappedVectorInt{*list1}, list2_const));
    EXPECT_EQ(exp, ranges::equal(WrappedVectorIntConst{list2_const}, *list1));
    EXPECT_EQ(exp, ranges::equal(WrappedVectorIntConst{list2_const},
                                 WrappedVectorIntConst{*list1}));
  }

  std::vector<int> list_empty;
  std::vector<int> list1;
  std::vector<int> list1_longer;
  std::vector<int> list2;
};

TEST_F(RangesUtilityTest, Empty) {
  // size() supported.
  EXPECT_TRUE(ranges::empty(list_empty));
  EXPECT_FALSE(ranges::empty(list1));
  EXPECT_FALSE(ranges::empty(list2));

  // size() unsupported.
  EXPECT_TRUE(ranges::empty(WrappedVectorInt{list_empty}));
  EXPECT_FALSE(ranges::empty(WrappedVectorInt{list1}));
  EXPECT_FALSE(ranges::empty(WrappedVectorInt{list2}));
}

TEST_F(RangesUtilityTest, Size) {
  // size() supported.
  EXPECT_EQ(0U, ranges::size(list_empty));
  EXPECT_EQ(7U, ranges::size(list1));

  // size() unsupported.
  EXPECT_EQ(0U, ranges::size(WrappedVectorInt{list_empty}));
  EXPECT_EQ(7U, ranges::size(WrappedVectorInt{list1}));
}

TEST_F(RangesUtilityTest, Equal) {
  // Equality with self.
  RangesUtilityTest::HelperEqual(&list_empty, &list_empty, true);
  RangesUtilityTest::HelperEqual(&list1, &list1, true);

  // Inequality with same size.
  RangesUtilityTest::HelperEqual(&list1, &list2, false);

  // Inequality with different sizes.
  RangesUtilityTest::HelperEqual(&list1, &list1_longer, false);
  RangesUtilityTest::HelperEqual(&list1_longer, &list1, false);
  RangesUtilityTest::HelperEqual(&list_empty, &list1, false);
  RangesUtilityTest::HelperEqual(&list1, &list_empty, false);
}

}  // namespace ranges
}  // namespace zucchini
