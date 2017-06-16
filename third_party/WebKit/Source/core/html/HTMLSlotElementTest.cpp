// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/html/HTMLSlotElement.h"

#include <array>

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

namespace {
constexpr int kSize = 16;
}

class HTMLSlotElementTest : public testing::Test {
 protected:
  HTMLSlotElementTest() {}

  int RunDPWith(const std::vector<char>& nodes1,
                const std::vector<char>& nodes2);
  inline std::pair<size_t, size_t> Pair(int r, int c) { return {r, c}; }

  std::array<std::array<int, kSize>, kSize> cost_table_;
  std::array<std::array<std::pair<size_t, size_t>, kSize>, kSize>
      back_ref_table_;
};

int HTMLSlotElementTest::RunDPWith(const std::vector<char>& nodes1,
                                   const std::vector<char>& nodes2) {
  HTMLSlotElement::FillEditDistanceDynamicProgrammingTable(
      nodes1, nodes2, cost_table_, back_ref_table_);
  return cost_table_[nodes1.size()][nodes2.size()];
}

TEST_F(HTMLSlotElementTest, FillEditDistanceDynamicProgrammingTableCost) {
  {
    std::vector<char> nodes1{};
    std::vector<char> nodes2{};
    EXPECT_EQ(RunDPWith(nodes1, nodes2), 0);
  }
  {
    std::vector<char> nodes1{'a'};
    std::vector<char> nodes2{};
    EXPECT_EQ(RunDPWith(nodes1, nodes2), 1);
  }
  {
    std::vector<char> nodes1{};
    std::vector<char> nodes2{'a'};
    EXPECT_EQ(RunDPWith(nodes1, nodes2), 1);
  }
  {
    std::vector<char> nodes1{'a'};
    std::vector<char> nodes2{'a'};
    EXPECT_EQ(RunDPWith(nodes1, nodes2), 0);
  }
  {
    std::vector<char> nodes1{'a', 'b'};
    std::vector<char> nodes2{'a'};
    EXPECT_EQ(RunDPWith(nodes1, nodes2), 1);
  }
  {
    std::vector<char> nodes1{'a', 'b'};
    std::vector<char> nodes2{'b', 'a'};
    EXPECT_EQ(RunDPWith(nodes1, nodes2), 2);
  }
  {
    std::vector<char> nodes1{'a', 'b', 'c', 'd'};
    std::vector<char> nodes2{};
    EXPECT_EQ(RunDPWith(nodes1, nodes2), 4);
  }
  {
    std::vector<char> nodes1{'a', 'b', 'c', 'd'};
    std::vector<char> nodes2{'a', 'b'};
    EXPECT_EQ(RunDPWith(nodes1, nodes2), 2);
  }
  {
    std::vector<char> nodes1{'a', 'b', 'c', 'd'};
    std::vector<char> nodes2{'a', 'd'};
    EXPECT_EQ(RunDPWith(nodes1, nodes2), 2);
  }
  {
    std::vector<char> nodes1{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
    std::vector<char> nodes2{'g', 'a', 'b', '1', 'd', '2', '3', 'h', '4'};

    // [a, b, d, h] should be (one of) the longest common sequences.
    std::vector<char> edited{'c', 'e', 'f', 'g', 'g',
                             '1', '2', '3', '4'};  // Use 'g' twice
                                                   // because it should be
                                                   // removed and
                                                   // inserted.
    EXPECT_EQ(RunDPWith(nodes1, nodes2), static_cast<int>(edited.size()));
  }
}

TEST_F(HTMLSlotElementTest, FillEditDistanceDynamicProgrammingTableBackRef) {
  std::vector<char> nodes1{'a', 'b', 'c', 'd'};
  std::vector<char> nodes2{'e', 'a', 'f', 'b'};

  EXPECT_EQ(RunDPWith(nodes1, nodes2), 4);

  // Cost table should be:
  //     0 1 2 3 4
  //       e a f b
  // 0   0 1 2 3 4
  // 1 a 1 2 1 2 3
  // 2 b 2 3 2 3 2
  // 3 c 3 4 3 4 3
  // 4 d 4 5 4 5 4

  EXPECT_EQ(cost_table_[4][4], 4);
  EXPECT_EQ(back_ref_table_[4][4], Pair(3, 4));

  EXPECT_EQ(cost_table_[3][4], 3);
  EXPECT_EQ(back_ref_table_[3][4], Pair(2, 4));

  EXPECT_EQ(cost_table_[2][4], 2);
  EXPECT_EQ(back_ref_table_[2][4], Pair(1, 3));

  EXPECT_EQ(cost_table_[1][3], 2);
  EXPECT_EQ(back_ref_table_[1][3], Pair(1, 2));

  EXPECT_EQ(cost_table_[1][2], 1);
  EXPECT_EQ(back_ref_table_[1][2], Pair(0, 1));

  // FillEditDistanceDynamicProgrammingTable doesn't fill BackRefTable for r ==
  // 0 or c == 0
  EXPECT_EQ(cost_table_[0][1], 1);
  EXPECT_EQ(cost_table_[0][0], 0);
}

}  // namespace blink
