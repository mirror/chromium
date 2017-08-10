// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/zucchini_gen.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

namespace {

constexpr auto BAD = kUnusedIndex;
using OffsetVector = std::vector<offset_t>;

}  // namespace

TEST(ZucchiniGenTest, MakeNewLabelsFromEquivalenceMap) {
  EXPECT_EQ(OffsetVector(), MakeNewLabelsFromEquivalenceMap({}, {}));
  EXPECT_EQ(OffsetVector({BAD, BAD}),
            MakeNewLabelsFromEquivalenceMap({0, 1}, {}));

  EXPECT_EQ(OffsetVector({0, 1, BAD}),
            MakeNewLabelsFromEquivalenceMap({0, 1, 2}, {{0, 0, 2}}));
  EXPECT_EQ(OffsetVector({1, 2, BAD}),
            MakeNewLabelsFromEquivalenceMap({0, 1, 2}, {{0, 1, 2}}));
  EXPECT_EQ(OffsetVector({1, BAD, 4, 5, 6, BAD}),
            MakeNewLabelsFromEquivalenceMap({0, 1, 2, 3, 4, 5},
                                            {{0, 1, 1}, {2, 4, 3}}));
  EXPECT_EQ(OffsetVector({1, BAD, 0, 1, 2, BAD}),
            MakeNewLabelsFromEquivalenceMap({0, 1, 2, 3, 4, 5},
                                            {{0, 1, 1}, {2, 0, 3}}));

  // Overlap.
  EXPECT_EQ(
      OffsetVector({1, 2, 3, BAD, BAD}),
      MakeNewLabelsFromEquivalenceMap({0, 1, 2, 3, 4}, {{0, 1, 3}, {1, 3, 2}}));
  EXPECT_EQ(
      OffsetVector({1, 3, 4, 5, BAD}),
      MakeNewLabelsFromEquivalenceMap({0, 1, 2, 3, 4}, {{0, 1, 2}, {1, 3, 3}}));
  EXPECT_EQ(
      OffsetVector({1, 2, 4, BAD, BAD}),
      MakeNewLabelsFromEquivalenceMap({0, 1, 2, 3, 4}, {{0, 1, 2}, {1, 3, 2}}));

  // Jump.
  EXPECT_EQ(OffsetVector({5, BAD, 6}),
            MakeNewLabelsFromEquivalenceMap(
                {10, 13, 15}, {{0, 1, 2}, {9, 4, 2}, {15, 6, 2}}));
}

TEST(ZucchiniGenTest, FindExtraLabels) {
  EXPECT_EQ(OffsetVector(), FindExtraLabels({}, {}));
  EXPECT_EQ(OffsetVector(), FindExtraLabels({{0, 0}}, {}));
  EXPECT_EQ(OffsetVector(), FindExtraLabels({{0, IsMarked(0)}}, {}));

  EXPECT_EQ(OffsetVector({0}),
            FindExtraLabels({{0, 0}}, EquivalenceMap({{0, 0, 2, 0.0}})));
  EXPECT_EQ(OffsetVector(), FindExtraLabels({{0, MarkIndex(0)}},
                                            EquivalenceMap({{0, 0, 2, 0.0}})));

  EXPECT_EQ(OffsetVector({1, 2}),
            FindExtraLabels({{0, 0}, {1, 1}, {2, 2}, {3, 3}},
                            EquivalenceMap({{0, 1, 2, 0.0}})));
  EXPECT_EQ(OffsetVector({2}),
            FindExtraLabels({{0, 0}, {1, MarkIndex(1)}, {2, 2}, {3, 3}},
                            EquivalenceMap({{0, 1, 2, 0.0}})));

  EXPECT_EQ(
      OffsetVector({1, 2, 4, 5}),
      FindExtraLabels({{0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}, {6, 6}},
                      EquivalenceMap({{0, 1, 2, 0.0}, {0, 4, 2, 0.0}})));
  EXPECT_EQ(OffsetVector({4, 5}),
            FindExtraLabels({{3, 3}, {4, 4}, {5, 5}, {6, 6}},
                            EquivalenceMap({{0, 1, 2, 0.0}, {0, 4, 2, 0.0}})));
}

}  // namespace zucchini
