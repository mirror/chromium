// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/zucchini/targets_affinity.h"

#include <stddef.h>

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
                          double min_affinity,
                          const std::vector<offset_t>& expected_old_labels,
                          const std::vector<offset_t>& expected_new_labels) {
        targets_affinity.InferFromSimilarities(equivalence_map, old_targets,
                                               new_targets);
        std::vector<offset_t> old_labels;
        std::vector<offset_t> new_labels;
        size_t bound = targets_affinity.AssignLabels(min_affinity, &old_labels,
                                                     &new_labels);
        EXPECT_EQ(expected_old_labels, old_labels);
        EXPECT_EQ(expected_new_labels, new_labels);
        return bound;
      };

  EXPECT_EQ(1U, test_labels_assignement(EquivalenceMap(), {}, {}, 1.0, {}, {}));
  EXPECT_EQ(1U, test_labels_assignement(EquivalenceMap({{{0, 0, 8}, 1.0}}), {},
                                        {}, 1.0, {}, {}));

  EXPECT_EQ(1U, test_labels_assignement(EquivalenceMap(), {0, 10}, {0, 5}, 1.0,
                                        {0, 0}, {0, 0}));

  EXPECT_EQ(2U, test_labels_assignement(EquivalenceMap({{{0, 0, 1}, 1.0}}),
                                        {0, 10}, {0, 5}, 1.0, {1, 0}, {1, 0}));
  EXPECT_EQ(1U, test_labels_assignement(EquivalenceMap({{{0, 0, 1}, 0.99}}),
                                        {0, 10}, {0, 5}, 1.0, {0, 0}, {0, 0}));
  EXPECT_EQ(1U, test_labels_assignement(EquivalenceMap({{{0, 0, 1}, 1.0}}),
                                        {0, 10}, {0, 5}, 1.01, {0, 0}, {0, 0}));
  EXPECT_EQ(1U, test_labels_assignement(EquivalenceMap({{{0, 0, 1}, 1.0}}),
                                        {0, 10}, {0, 5}, 15.0, {0, 0}, {0, 0}));
  EXPECT_EQ(2U, test_labels_assignement(EquivalenceMap({{{0, 0, 1}, 15.0}}),
                                        {0, 10}, {0, 5}, 15.0, {1, 0}, {1, 0}));

  EXPECT_EQ(2U, test_labels_assignement(EquivalenceMap({{{0, 1, 2}, 1.0}}),
                                        {0, 10}, {1, 5}, 1.0, {1, 0}, {1, 0}));
  EXPECT_EQ(
      3U, test_labels_assignement(EquivalenceMap({{{0, 0, 6}, 1.0}}),
                                  {0, 5, 10}, {0, 5}, 1.0, {1, 2, 0}, {1, 2}));
  EXPECT_EQ(3U, test_labels_assignement(
                    EquivalenceMap({{{5, 0, 2}, 1.0}, {{0, 5, 2}, 2.0}}),
                    {0, 5, 10}, {0, 5}, 1.0, {1, 2, 0}, {2, 1}));
  EXPECT_EQ(2U, test_labels_assignement(
                    EquivalenceMap({{{0, 0, 2}, 1.0}, {{0, 5, 2}, 2.0}}),
                    {0, 5, 10}, {0, 5}, 1.0, {1, 0, 0}, {0, 1}));
}

}  // namespace zucchini
