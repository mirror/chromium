// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/labeled_pool.h"

#include "base/test/gtest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

class LabeledPoolTest : public testing::Test {
 protected:
  static const std::vector<Reference> kBaseReferenceSet;
  static const std::vector<Reference> kExtraReferenceSet;

  void SetUp() override { labeled_pool_.InsertTargets(kBaseReferenceSet); }

  LabeledPool labeled_pool_;
};

const std::vector<Reference> LabeledPoolTest::kBaseReferenceSet = {
    {0, 0},
    {1, 2},
    {1, 0},
    {1, 4},
};
const std::vector<Reference> LabeledPoolTest::kExtraReferenceSet = {
    {0, 3},
    {1, 2},
    {2, 5},
};

TEST_F(LabeledPoolTest, InsertSymbols) {
  EXPECT_EQ(3, labeled_pool_.size());
  EXPECT_EQ(std::vector<offset_t>({0, 2, 4}), labeled_pool_.targets());

  labeled_pool_.InsertTargets(kExtraReferenceSet);
  EXPECT_EQ(5, labeled_pool_.size());
  EXPECT_EQ(std::vector<offset_t>({0, 2, 3, 4, 5}), labeled_pool_.targets());
}

TEST_F(LabeledPoolTest, FindIndex) {
  offset_t index = 0;
  for (uint32_t target : labeled_pool_.targets()) {
    EXPECT_EQ(index, labeled_pool_.FindIndexForTarget(target));
    ++index;
  }
  EXPECT_FALSE(labeled_pool_.FindIndexForTarget(1));
}

TEST_F(LabeledPoolTest, ResetLabels) {
  EXPECT_DCHECK_DEATH(labeled_pool_.ResetLabels({1}, 2));

  labeled_pool_.ResetLabels({0, 1, 2}, 3);
  EXPECT_EQ(std::vector<offset_t>({0, 1, 2}), labeled_pool_.labels());
  EXPECT_EQ(3, labeled_pool_.label_bound());
  labeled_pool_.ResetLabels({1, 3, 1}, 4);
  EXPECT_EQ(std::vector<offset_t>({1, 3, 1}), labeled_pool_.labels());
  EXPECT_EQ(4, labeled_pool_.label_bound());
}

}  // namespace zucchini
