// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/reference_set.h"

#include <vector>

#include "chrome/installer/zucchini/target_pool.h"
#include "chrome/installer/zucchini/test_reference_reader.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

namespace {

using OffsetVector = std::vector<offset_t>;

}  // namespace

class ReferenceSetTest : public testing::Test {
 protected:
  ReferenceSetTest() : reference_set_({2, TypeTag(0), PoolTag(0)}) {
    target_pool_.Init({0, 2, 3, 5});
  }

  ReferenceSet reference_set_;
  TargetPool target_pool_;
};

TEST_F(ReferenceSetTest, InsertReferencesFromReader) {
  EXPECT_EQ(std::vector<IndirectReference>(), reference_set_.references());
  EXPECT_EQ(0U, reference_set_.size());
  std::vector<Reference> references = {{0, 0}, {2, 2}, {4, 5}};
  reference_set_.InsertReferences(TestReferenceReader(references),
                                  target_pool_);
  EXPECT_EQ(std::vector<IndirectReference>({{0, 0}, {2, 1}, {4, 3}}),
            reference_set_.references());
  EXPECT_EQ(3U, reference_set_.size());
}

TEST_F(ReferenceSetTest, At) {
  reference_set_.InsertReferences({{0, 0}, {2, 2}, {5, 5}}, target_pool_);
  EXPECT_EQ(IndirectReference({0, 0}), reference_set_.at(0));
  EXPECT_EQ(IndirectReference({0, 0}), reference_set_.at(1));
  EXPECT_EQ(IndirectReference({2, 1}), reference_set_.at(2));
  EXPECT_EQ(IndirectReference({2, 1}), reference_set_.at(3));
  EXPECT_EQ(IndirectReference({5, 3}), reference_set_.at(5));
  EXPECT_EQ(IndirectReference({5, 3}), reference_set_.at(6));
}

}  // namespace zucchini
