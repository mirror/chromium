// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/targets_affinity.h"

#include <type_traits>
#include <vector>

#include "chrome/installer/zucchini/equivalence_map.h"
#include "chrome/installer/zucchini/image_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

TEST(TargetsAffinityTest, AffinityBetween) {
  TargetsAffinity targets_affinity;

  auto test_affinity = [&targets_affinity](
                           const EquivalenceMap& equivalence_map,
                           const std::vector<offset_t>& old_targets,
                           const std::vector<offset_t>& new_targets) {
    targets_affinity.InferFromSimilarities(equivalence_map, old_targets,
                                           new_targets);
    std::vector<double> affinities;
    for (size_t i = 0; i < old_targets.size(); ++i) {
      for (size_t j = 0; j < new_targets.size(); ++j) {
        affinities.push_back(targets_affinity.AffinityBetween(i, j));
      }
    }
    return affinities;
  };

  EXPECT_EQ(std::vector<double>({}), test_affinity(EquivalenceMap(), {}, {}));
  EXPECT_EQ(std::vector<double>({}),
            test_affinity(EquivalenceMap({{{0, 0, 8}, 1.0}}), {}, {}));

  EXPECT_EQ(std::vector<double>({0.0, 0.0, 0.0, 0.0}),
            test_affinity(EquivalenceMap(), {0, 10}, {0, 5}));

  EXPECT_EQ(std::vector<double>({1.0, -1.0, -1.0, 0.0}),
            test_affinity(EquivalenceMap({{{0, 0, 1}, 1.0}}), {0, 10}, {0, 5}));

  EXPECT_EQ(std::vector<double>({1.0, -1.0, -1.0, 0.0}),
            test_affinity(EquivalenceMap({{{0, 0, 2}, 1.0}}), {1, 10}, {1, 5}));

  EXPECT_EQ(std::vector<double>({0.0, 0.0, 0.0, 0.0}),
            test_affinity(EquivalenceMap({{{0, 1, 2}, 1.0}}), {1, 10}, {1, 5}));

  EXPECT_EQ(std::vector<double>({1.0, -1.0, -1.0, 0.0}),
            test_affinity(EquivalenceMap({{{0, 1, 2}, 1.0}}), {0, 10}, {1, 5}));

  EXPECT_EQ(std::vector<double>({2.0, -2.0, -2.0, 0.0}),
            test_affinity(EquivalenceMap({{{0, 0, 1}, 2.0}}), {0, 10}, {0, 5}));

  EXPECT_EQ(
      std::vector<double>({1.0, -1.0, -1.0, 1.0, -1.0, -1.0}),
      test_affinity(EquivalenceMap({{{0, 0, 6}, 1.0}}), {0, 5, 10}, {0, 5}));

  EXPECT_EQ(std::vector<double>({-2.0, 2.0, 1.0, -2.0, -1.0, -2.0}),
            test_affinity(EquivalenceMap({{{5, 0, 2}, 1.0}, {{0, 5, 2}, 2.0}}),
                          {0, 5, 10}, {0, 5}));

  EXPECT_EQ(std::vector<double>({-2.0, 2.0, 0.0, -2.0, 0.0, -2.0}),
            test_affinity(EquivalenceMap({{{0, 0, 2}, 1.0}, {{0, 5, 2}, 2.0}}),
                          {0, 5, 10}, {0, 5}));
}

TEST(TargetsAffinityTest, AssignLabels) {
  TargetsAffinity targets_affinity;

  auto test_labels_assignement =
      [&targets_affinity](const EquivalenceMap& equivalence_map,
                          const std::vector<offset_t>& old_targets,
                          const std::vector<offset_t>& new_targets,
                          const std::vector<size_t>& expected_old_labels,
                          const std::vector<size_t>& expected_new_labels) {
        targets_affinity.InferFromSimilarities(equivalence_map, old_targets,
                                               new_targets);
        std::vector<size_t> old_labels;
        std::vector<size_t> new_labels;
        targets_affinity.AssignLabels(1.0, &old_labels, &new_labels);
        EXPECT_EQ(expected_old_labels, old_labels);
        EXPECT_EQ(expected_new_labels, new_labels);
      };

  test_labels_assignement(EquivalenceMap(), {}, {}, {}, {});
  test_labels_assignement(EquivalenceMap({{{0, 0, 8}, 1.0}}), {}, {}, {}, {});

  test_labels_assignement(EquivalenceMap(), {0, 10}, {0, 5}, {0, 0}, {0, 0});
  test_labels_assignement(EquivalenceMap({{{0, 0, 1}, 0.99}}), {0, 10}, {0, 5},
                          {0, 0}, {0, 0});
  test_labels_assignement(EquivalenceMap({{{0, 0, 1}, 1.0}}), {0, 10}, {0, 5},
                          {1, 0}, {1, 0});
  test_labels_assignement(EquivalenceMap({{{0, 1, 2}, 1.0}}), {0, 10}, {1, 5},
                          {1, 0}, {1, 0});
  test_labels_assignement(EquivalenceMap({{{0, 0, 6}, 1.0}}), {0, 5, 10},
                          {0, 5}, {1, 2, 0}, {1, 2});
  test_labels_assignement(EquivalenceMap({{{5, 0, 2}, 1.0}, {{0, 5, 2}, 2.0}}),
                          {0, 5, 10}, {0, 5}, {1, 2, 0}, {2, 1});
  test_labels_assignement(EquivalenceMap({{{0, 0, 2}, 1.0}, {{0, 5, 2}, 2.0}}),
                          {0, 5, 10}, {0, 5}, {1, 0, 0}, {0, 1});
}

}  // namespace zucchini
