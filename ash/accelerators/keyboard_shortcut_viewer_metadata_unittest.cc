// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "ash/accelerators/accelerator_table.h"
#include "ash/accelerators/keyboard_shortcut_viewer_metadata.h"
#include "ash/ash_export.h"
#include "ash/test/ash_test_base.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ash {

namespace {

std::string ActionsToString(
    const std::vector<ash::AcceleratorAction>& actions) {
  std::vector<std::string> msgs;
  for (const auto& action : actions) {
    msgs.emplace_back(base::StringPrintf("action=%d", action));
  }
  return base::JoinString(msgs, ", ");
}

class KeyboardShortcutViewerMetadataTest : public AshTestBase {
 public:
  KeyboardShortcutViewerMetadataTest() = default;
  ~KeyboardShortcutViewerMetadataTest() override = default;

  void SetUp() override {
    for (size_t i = 0; i < kAcceleratorDataLength; ++i) {
      const AcceleratorData& accel_data = kAcceleratorData[i];
      action_to_accelerator_data_[accel_data.action].emplace_back(accel_data);
    }
    AshTestBase::SetUp();
  }

 protected:
  std::map<AcceleratorAction, std::vector<AcceleratorData>>
      action_to_accelerator_data_;

 private:
  DISALLOW_COPY_AND_ASSIGN(KeyboardShortcutViewerMetadataTest);
};

}  // namespace

// Test that grouped AcceleratorAction should have same modifiers.
TEST_F(KeyboardShortcutViewerMetadataTest,
       CheckModifiersEqualForGroupedActions) {
  for (size_t i = 0; i < kKSVItemInfoToActionsLength; ++i) {
    const KSVItemInfoToActions& entry = kKSVItemInfoToActions[i];
    // This test only checks grouped actions.
    if (entry.actions.size() <= 1)
      continue;

    bool first_action = true;
    int modifiers;
    for (const auto& action : entry.actions) {
      auto iter = action_to_accelerator_data_.find(action);
      ASSERT_TRUE(iter != action_to_accelerator_data_.end());

      for (const auto& accel_data : iter->second) {
        if (first_action) {
          modifiers = accel_data.modifiers;
          first_action = false;
        } else {
          EXPECT_EQ(modifiers, accel_data.modifiers)
              << "Grouped actions should have same modifiers: "
              << ActionsToString(entry.actions);
        }
      }
    }
  }
}

// Test that if the replacement is empty, the AcceleratorAction cannot have more
// than one corresponding AcceleratorData.
TEST_F(KeyboardShortcutViewerMetadataTest,
       CheckReplacementsForActionWithMultipleAcceleratorData) {
  for (size_t i = 0; i < kKSVItemInfoToActionsLength; ++i) {
    const KSVItemInfoToActions& entry = kKSVItemInfoToActions[i];
    // This test only checks entries with empty replacement.
    if (!entry.shortcut_info.replacement_ids.empty())
      continue;

    for (const auto& action : entry.actions) {
      auto iter = action_to_accelerator_data_.find(action);
      ASSERT_TRUE(iter != action_to_accelerator_data_.end());
      EXPECT_TRUE(iter->second.size() == 1)
          << "The replacement is empty. The AcceleratorAction cannot have more "
          << "than one corresponding AcceleratorData. "
          << "action=" << action;
    }
  }
}

}  // namespace ash
