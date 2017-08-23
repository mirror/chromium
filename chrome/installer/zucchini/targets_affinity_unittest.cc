// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/targets_affinity.h"

#include <type_traits>

#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

class TargetsAffinityTest : public testing::Test {
 protected:
  TargetsAffinity affinity_;
};

TEST_F(TargetsAffinityTest, InferFromSimilarities) {
  affinity_.InferFromSimilarities({}, {0, 5, 10}, {0, 5});
  EXPECT_EQ(0.0, affinity_.AffinityBetween(0, 0));
  EXPECT_EQ(0.0, affinity_.AffinityBetween(0, 1));
  EXPECT_EQ(0.0, affinity_.AffinityBetween(1, 0));
  EXPECT_EQ(0.0, affinity_.AffinityBetween(1, 1));
  EXPECT_EQ(0.0, affinity_.AffinityBetween(2, 0));
  EXPECT_EQ(0.0, affinity_.AffinityBetween(2, 1));

  affinity_.InferFromSimilarities({{0, 0, 8, 1.0}}, {0, 5, 10}, {0, 5});
  EXPECT_EQ(1.0, affinity_.AffinityBetween(0, 0));
  EXPECT_EQ(-1.0, affinity_.AffinityBetween(0, 1));
  EXPECT_EQ(1.0, affinity_.AffinityBetween(1, 1));
  EXPECT_EQ(-1.0, affinity_.AffinityBetween(2, 1));

  affinity_.InferFromSimilarities({{5, 0, 2, 1.0}, {0, 5, 2, 2.0}}, {0, 5, 10},
                                  {0, 5});
  EXPECT_EQ(1.0, affinity_.AffinityBetween(1, 0));
  EXPECT_EQ(-2.0, affinity_.AffinityBetween(0, 0));
  EXPECT_EQ(2.0, affinity_.AffinityBetween(0, 1));
  EXPECT_EQ(-2.0, affinity_.AffinityBetween(1, 1));
  EXPECT_EQ(-2.0, affinity_.AffinityBetween(2, 1));

  affinity_.InferFromSimilarities({{0, 0, 2, 1.0}, {0, 5, 2, 2.0}}, {0, 5, 10},
                                  {0, 5});
  EXPECT_EQ(-2.0, affinity_.AffinityBetween(0, 0));
  EXPECT_EQ(2.0, affinity_.AffinityBetween(0, 1));
}

}  // namespace zucchini
