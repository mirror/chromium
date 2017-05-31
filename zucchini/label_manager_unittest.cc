// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "zucchini/label_manager.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace zucchini {

TEST(UnorderedLabelManagerTest, Init) {
  std::vector<offset_t> labels = {0, kUnusedIndex, 3, kUnusedIndex,
                                  1, kUnusedIndex, 5};

  UnorderedLabelManager label_manager;
  label_manager.Init(std::move(labels));

  std::vector<offset_t> expected = {0, kUnusedIndex, 3, kUnusedIndex,
                                    1, kUnusedIndex, 5};
  EXPECT_EQ(expected, label_manager.Labels());
}

TEST(UnorderedLabelManagerTest, AssignOrAllocate) {
  std::vector<offset_t> input = {1, 3, 5, MarkIndex(3), 0};

  UnorderedLabelManager label_manager;
  label_manager.AssignOrAllocate(input);

  std::vector<offset_t> expected_labels = {0, 1, 3, 5};
  std::vector<offset_t> expected_output = {1, 3, 5, MarkIndex(3), 0};
  EXPECT_EQ(expected_labels, label_manager.Labels());
  EXPECT_EQ(expected_output, input);
}

TEST(UnorderedLabelManagerTest, InitAssignOrAllocate) {
  std::vector<offset_t> labels = {0, kUnusedIndex, 3, kUnusedIndex,
                                  1, kUnusedIndex, 5};
  std::vector<offset_t> input = {2, 3, 4, MarkIndex(3), 1};

  UnorderedLabelManager label_manager;
  label_manager.Init(std::move(labels));
  label_manager.AssignOrAllocate(input);

  std::vector<offset_t> expected_labels = {0, 2, 3, 4, 1, kUnusedIndex, 5};
  std::vector<offset_t> expected_output = {2, MarkIndex(2), 4, MarkIndex(3),
                                           MarkIndex(4)};

  EXPECT_EQ(expected_labels, label_manager.Labels());
  EXPECT_EQ(expected_output, input);
}

TEST(UnorderedLabelManagerTest, AllocateAndAssign) {
  std::vector<offset_t> input = {1, 3, 5, MarkIndex(3), 0};

  UnorderedLabelManager label_manager;
  label_manager.AllocateAndAssign(input);

  std::vector<offset_t> expected_labels = {0, 1, 3, 5};
  std::vector<offset_t> expected_output = {1, 2, 3, 3, 0};

  std::for_each(expected_output.begin(), expected_output.end(),
                [](offset_t& value) { value = MarkIndex(value); });
  EXPECT_EQ(expected_labels, label_manager.Labels());
  EXPECT_EQ(expected_output, input);
}

TEST(UnorderedLabelManagerTest, InitAllocateAndAssignUnfilled) {
  std::vector<offset_t> labels = {0, kUnusedIndex, 3, kUnusedIndex,
                                  1, kUnusedIndex, 5};
  std::vector<offset_t> input = {2, 3, 4, MarkIndex(3), 1};

  UnorderedLabelManager label_manager;
  label_manager.Init(std::move(labels));
  label_manager.AllocateAndAssign(input);

  std::vector<offset_t> expected_labels = {0, 2, 3, 4, 1, kUnusedIndex, 5};
  std::vector<offset_t> expected_output = {1, 2, 3, 3, 4};
  std::for_each(expected_output.begin(), expected_output.end(),
                [](offset_t& value) { value = MarkIndex(value); });

  EXPECT_EQ(expected_labels, label_manager.Labels());
  EXPECT_EQ(expected_output, input);
}

TEST(UnorderedLabelManagerTest, InitAllocateAndAssignFilled) {
  std::vector<offset_t> labels = {0, kUnusedIndex, 3, kUnusedIndex,
                                  1, kUnusedIndex, 5};
  std::vector<offset_t> input = {2, 3, 4, MarkIndex(3), 1, 7, 8};

  UnorderedLabelManager label_manager;
  label_manager.Init(std::move(labels));
  label_manager.AllocateAndAssign(input);

  std::vector<offset_t> expected_labels = {0, 2, 3, 4, 1, 7, 5, 8};
  std::vector<offset_t> expected_output = {1, 2, 3, 3, 4, 5, 7};

  std::for_each(expected_output.begin(), expected_output.end(),
                [](offset_t& value) { value = MarkIndex(value); });
  EXPECT_EQ(expected_labels, label_manager.Labels());
  EXPECT_EQ(expected_output, input);
}

TEST(UnorderedLabelManagerTest, AssignUnassign) {
  std::vector<offset_t> labels = {0, kUnusedIndex, 3, kUnusedIndex,
                                  1, kUnusedIndex, 5};
  std::vector<offset_t> input = {2, 3, 4, MarkIndex(4), 1};

  UnorderedLabelManager label_manager;
  label_manager.Init(std::move(labels));
  std::vector<offset_t> output = input;
  label_manager.Assign(output);

  std::vector<offset_t> expected_labels = {0, kUnusedIndex, 3, kUnusedIndex,
                                           1, kUnusedIndex, 5};
  std::vector<offset_t> expected_output = {2, MarkIndex(2), 4, MarkIndex(4),
                                           MarkIndex(4)};

  EXPECT_EQ(expected_labels, label_manager.Labels());
  EXPECT_EQ(expected_output, output);

  input[3] = 1;
  label_manager.Unassign(output);
  EXPECT_EQ(input, output);
}

// TODO(etiennep): Test OrderedLabelManager as well.

}  // namespace zucchini
