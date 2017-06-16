// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/HTMLSlotElement.h"

#include <array>

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {
constexpr int kTableSize = 16 + 1;
}

class HTMLSlotElementTest : public testing::Test {
 protected:
  HTMLSlotElementTest() {}

  int LongestCommonSubsequenceSize(const std::vector<char>& nodes1,
                                   const std::vector<char>& nodes2);
  inline std::pair<size_t, size_t> Pair(int r, int c) { return {r, c}; }

  std::array<std::array<int, kTableSize>, kTableSize> lcs_table_;
  std::array<std::array<std::pair<size_t, size_t>, kTableSize>, kTableSize>
      backtrack_table_;
};

int HTMLSlotElementTest::LongestCommonSubsequenceSize(
    const std::vector<char>& nodes1,
    const std::vector<char>& nodes2) {
  HTMLSlotElement::FillLongestCommonSubsequenceDynamicProgrammingTable(
      nodes1, nodes2, lcs_table_, backtrack_table_);
  return lcs_table_[nodes1.size()][nodes2.size()];
}

TEST_F(HTMLSlotElementTest,
       FillLongestCommonSubsequenceDynamicProgrammingTableLength) {
  {
    std::vector<char> nodes1{};
    std::vector<char> nodes2{};
    EXPECT_EQ(LongestCommonSubsequenceSize(nodes1, nodes2), 0);
  }
  {
    std::vector<char> nodes1{'a'};
    std::vector<char> nodes2{};
    EXPECT_EQ(LongestCommonSubsequenceSize(nodes1, nodes2), 0);
  }
  {
    std::vector<char> nodes1{};
    std::vector<char> nodes2{'a'};
    EXPECT_EQ(LongestCommonSubsequenceSize(nodes1, nodes2), 0);
  }
  {
    std::vector<char> nodes1{'a'};
    std::vector<char> nodes2{'a'};
    EXPECT_EQ(LongestCommonSubsequenceSize(nodes1, nodes2), 1);
  }
  {
    std::vector<char> nodes1{'a', 'b'};
    std::vector<char> nodes2{'a'};
    EXPECT_EQ(LongestCommonSubsequenceSize(nodes1, nodes2), 1);
  }
  {
    std::vector<char> nodes1{'a', 'b'};
    std::vector<char> nodes2{'b', 'a'};
    EXPECT_EQ(LongestCommonSubsequenceSize(nodes1, nodes2), 1);
  }
  {
    std::vector<char> nodes1{'a', 'b', 'c', 'd'};
    std::vector<char> nodes2{};
    EXPECT_EQ(LongestCommonSubsequenceSize(nodes1, nodes2), 0);
  }
  {
    std::vector<char> nodes1{'a', 'b', 'c', 'd'};
    std::vector<char> nodes2{'1', 'a', 'b', 'd'};
    EXPECT_EQ(LongestCommonSubsequenceSize(nodes1, nodes2), 3);
  }
  {
    std::vector<char> nodes1{'a', 'b', 'c', 'd'};
    std::vector<char> nodes2{'b', 'a', 'c'};
    EXPECT_EQ(LongestCommonSubsequenceSize(nodes1, nodes2), 2);
  }
  {
    std::vector<char> nodes1{'a', 'b', 'c', 'd'};
    std::vector<char> nodes2{'1', 'b', '2', 'd', '1'};
    EXPECT_EQ(LongestCommonSubsequenceSize(nodes1, nodes2), 2);
  }
  {
    std::vector<char> nodes1{'a', 'b', 'c', 'd'};
    std::vector<char> nodes2{'a', 'd'};
    EXPECT_EQ(LongestCommonSubsequenceSize(nodes1, nodes2), 2);
  }
  {
    std::vector<char> nodes1{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    std::vector<char> nodes2{'g', 'a', 'b', '1', 'd', '2', '3', 'h', '4'};
    // [a, b, d, h] should be (one of) the longest common sebsequence.
    EXPECT_EQ(LongestCommonSubsequenceSize(nodes1, nodes2), 4);
  }
}

TEST_F(HTMLSlotElementTest,
       FillLongestCommonSubsequenceDynamicProgrammingTableBackTrack) {
  std::vector<char> nodes1{'a', 'b', 'c', 'd'};
  std::vector<char> nodes2{'e', 'a', 'f', 'b'};

  // [a, b] is the LCS
  EXPECT_EQ(LongestCommonSubsequenceSize(nodes1, nodes2), 2);

  // LCS table should be:
  //     0 1 2 3 4
  //       e a f b
  // 0   0 0 0 0 0
  // 1 a 0 0 1 1 1
  // 2 b 0 0 1 1 2
  // 3 c 0 0 1 1 2
  // 4 d 0 0 1 1 2

  EXPECT_EQ(lcs_table_[4][4], 2);
  EXPECT_EQ(backtrack_table_[4][4], Pair(3, 4));

  EXPECT_EQ(lcs_table_[3][4], 2);
  EXPECT_EQ(backtrack_table_[3][4], Pair(2, 4));

  EXPECT_EQ(lcs_table_[2][4], 2);
  EXPECT_EQ(backtrack_table_[2][4], Pair(1, 3));

  EXPECT_EQ(lcs_table_[1][3], 1);
  EXPECT_EQ(backtrack_table_[1][3], Pair(1, 2));

  EXPECT_EQ(lcs_table_[1][2], 1);
  EXPECT_EQ(backtrack_table_[1][2], Pair(0, 1));

  // FillLongestCommonSubsequenceDynamicProgrammingTable doesn't fill
  // BackTrackTable for r == 0 or c == 0
  EXPECT_EQ(lcs_table_[0][1], 0);
  EXPECT_EQ(lcs_table_[0][0], 0);
}

}  // namespace blink
