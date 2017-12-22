// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "ash/accelerators/accelerator_table.h"
#include "ash/accelerators/keyboard_shortcut_viewer_metadata.h"
#include "ash/ash_export.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ash {

namespace {

std::string ActionsToString(
    const std::vector<ash::AcceleratorAction>& actions) {
  std::vector<std::string> msgs;
  for (size_t i = 0; i < actions.size(); ++i) {
    msgs.emplace_back(base::StringPrintf("action=%d", actions[i]));
  }
  return base::JoinString(msgs, ", ");
}

}  // namespace

// Test that grouped AcceleratorAction should have same modifiers.
TEST(KeyboardShortcutViewerMetadataTest, CheckModifiersEqualForGroupedActions) {
  std::map<AcceleratorAction, AcceleratorData> action_to_accelerator_data;
  for (size_t i = 0; i < kAcceleratorDataLength; ++i) {
    const AcceleratorData& accel_data = kAcceleratorData[i];
    action_to_accelerator_data[accel_data.action] = accel_data;
  }

  for (size_t i = 0; i < kKSVItemInfoToActionsLength; ++i) {
    const KSVItemInfoToActions& entry = kKSVItemInfoToActions[i];
    // This test only checks grouped actions.
    if (entry.actions.size() <= 1)
      continue;

    bool first_action = true;
    int modifier;
    for (size_t j = 0; j < entry.actions.size(); ++j) {
      const AcceleratorAction& action = entry.actions[j];
      auto iter = action_to_accelerator_data.find(action);
      EXPECT_TRUE(iter != action_to_accelerator_data.end());

      const AcceleratorData& accel_data = iter->second;
      if (first_action) {
        modifier = accel_data.modifiers;
        first_action = false;
      } else {
        EXPECT_EQ(modifier, accel_data.modifiers)
            << "Grouped actions should have same modifiers: "
            << ActionsToString(entry.actions);
      }
    }
  }
}

// Test that if the replacement is empty, the AcceleratorAction cannot have more
// than one corresponding AcceleratorData.
TEST(KeyboardShortcutViewerMetadataTest,
     CheckReplacementsForActionWithMultipleAcceleratorData) {
  std::map<AcceleratorAction, std::vector<AcceleratorData>>
      action_to_accelerator_data;
  if (action_to_accelerator_data.empty()) {
    for (size_t i = 0; i < kAcceleratorDataLength; ++i) {
      const AcceleratorData& accel_data = kAcceleratorData[i];
      action_to_accelerator_data[accel_data.action].emplace_back(accel_data);
    }
  }

  for (size_t i = 0; i < kKSVItemInfoToActionsLength; ++i) {
    const KSVItemInfoToActions& entry = kKSVItemInfoToActions[i];
    // This test only checks entries with empty replacement.
    if (!entry.shortcut_info.replacement_ids.empty())
      continue;

    for (size_t i = 0; i < entry.actions.size(); ++i) {
      const AcceleratorAction& action = entry.actions[i];
      auto iter = action_to_accelerator_data.find(action);
      DCHECK(iter != action_to_accelerator_data.end());
      EXPECT_TRUE(iter->second.size() == 1)
          << "The replacement is empty. The AcceleratorAction cannot have more "
          << "than one corresponding AcceleratorData. "
          << "action=" << action;
    }
  }
}

}  // namespace ash
