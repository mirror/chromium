// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/target_pool.h"

#include <utility>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

namespace {

using OffsetVector = std::vector<offset_t>;

}  // namespace

TEST(TargetPoolTest, InsertTargetsFromReferences) {
  auto test_insert =
      [](std::vector<Reference>&& references) -> std::vector<offset_t> {
    TargetPool target_pool;
    target_pool.InsertTargets(references);
    return target_pool.targets();
  };

  EXPECT_EQ(OffsetVector(), test_insert({}));
  EXPECT_EQ(OffsetVector({0, 1}), test_insert({{0, 0}, {1, 1}}));
  EXPECT_EQ(OffsetVector({0, 1}), test_insert({{0, 1}, {1, 0}}));
  EXPECT_EQ(OffsetVector({0, 1, 2}), test_insert({{0, 1}, {1, 0}, {2, 2}}));
  EXPECT_EQ(OffsetVector({0}), test_insert({{0, 0}, {1, 0}}));
  EXPECT_EQ(OffsetVector({0, 1}), test_insert({{0, 0}, {1, 0}, {2, 1}}));
}

TEST(TargetPoolTest, KeyOffset) {
  auto test_key_offset = [](OffsetVector&& targets) {
    TargetPool target_pool;
    target_pool.Init(std::move(targets));
    for (offset_t offset : target_pool.targets()) {
      offset_t key = target_pool.KeyForOffset(offset);
      EXPECT_LT(key, target_pool.size());
      EXPECT_EQ(offset, target_pool.OffsetForKey(key));
    }
  };
  test_key_offset({0});
  test_key_offset({1});
  test_key_offset({0, 1});
  test_key_offset({0, 2});
  test_key_offset({1, 2});
  test_key_offset({1, 3});
  test_key_offset({1, 3, 7, 9, 13});
}

}  // namespace zucchini
