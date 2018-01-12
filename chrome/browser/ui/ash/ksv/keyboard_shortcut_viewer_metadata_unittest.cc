// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <set>
#include <string>
#include <vector>

#include "ash/accelerators/accelerator_table.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "chrome/browser/ui/views/accelerator_table.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/chromeos/ksv/keyboard_shortcut_viewer_metadata.h"

namespace ksv {

namespace {

std::string AcceleratorIdToString(const AcceleratorId& accelerator_id) {
  return base::StringPrintf(
      "keycode=%d shift=%s control=%s alt=%s search=%s", accelerator_id.keycode,
      (accelerator_id.modifiers & ui::EF_SHIFT_DOWN) ? "true" : "false",
      (accelerator_id.modifiers & ui::EF_CONTROL_DOWN) ? "true" : "false",
      (accelerator_id.modifiers & ui::EF_ALT_DOWN) ? "true" : "false",
      (accelerator_id.modifiers & ui::EF_COMMAND_DOWN) ? "true" : "false");
}

std::string AcceleratorIdsToString(
    const std::vector<AcceleratorId>& accelerator_ids) {
  std::vector<std::string> msgs;
  for (const auto& id : accelerator_ids)
    msgs.emplace_back(AcceleratorIdToString(id));
  return base::JoinString(msgs, ", ");
}

class KeyboardShortcutViewerMetadataTest : public testing::Test {
 public:
  KeyboardShortcutViewerMetadataTest() = default;
  ~KeyboardShortcutViewerMetadataTest() override = default;

  void SetUp() override {
    for (size_t i = 0; i < ash::kAcceleratorDataLength; ++i) {
      const ash::AcceleratorData& accel_data = ash::kAcceleratorData[i];
      ash_accelerator_ids_.insert({accel_data.keycode, accel_data.modifiers});
    }

    for (const auto& accel_mapping : GetAcceleratorList()) {
      chrome_accelerator_ids_.insert(
          {accel_mapping.keycode, accel_mapping.modifiers});
    }

    ksv_item_to_ids_list_ = GetKSVItemToAcceleratorIdsList();
    testing::Test::SetUp();
  }

 protected:
  // ash accelerator ids:
  std::set<AcceleratorId> ash_accelerator_ids_;

  // chrome accelerator ids:
  std::set<AcceleratorId> chrome_accelerator_ids_;

  // KSV metadata:
  std::vector<KSVItemToAcceleratorIds> ksv_item_to_ids_list_;

 private:
  DISALLOW_COPY_AND_ASSIGN(KeyboardShortcutViewerMetadataTest);
};

}  // namespace

// Test that AcceleratorId has at least one corresponding accelerators in ash or
// chrome.
TEST_F(KeyboardShortcutViewerMetadataTest, CheckAcceleratorIdHasAccelerator) {
  for (const auto& ksv_to_ids : ksv_item_to_ids_list_) {
    for (const auto& accelerator_id : ksv_to_ids.accelerator_ids) {
      int number_of_accelerator = ash_accelerator_ids_.count(accelerator_id) +
                                  chrome_accelerator_ids_.count(accelerator_id);
      EXPECT_GE(number_of_accelerator, 1)
          << "accelerator_id has no corresponding accelerator: "
          << AcceleratorIdToString(accelerator_id);
    }
  }
}

// Test that AcceleratorIds have no duplicates.
TEST_F(KeyboardShortcutViewerMetadataTest, CheckAcceleratorIdsNoDuplicates) {
  std::set<AcceleratorId> accelerator_ids_;
  for (const auto& ksv_to_ids : ksv_item_to_ids_list_) {
    for (const auto& accelerator_id : ksv_to_ids.accelerator_ids) {
      EXPECT_EQ(accelerator_ids_.count(accelerator_id), 0ul)
          << "Has duplicated accelerator_id: "
          << AcceleratorIdToString(accelerator_id);
      accelerator_ids_.insert(accelerator_id);
    }
  }
}

// Test that |replacement_codes| and |accelerator_ids| cannot be both empty.
TEST_F(KeyboardShortcutViewerMetadataTest,
       CheckReplacementCodesAndAcceleratorIdsNotBothEmpty) {
  for (const auto& ksv_to_ids : ksv_item_to_ids_list_) {
    EXPECT_TRUE(!ksv_to_ids.ksv_item.replacement_codes.empty() ||
                !ksv_to_ids.accelerator_ids.empty())
        << "replacement_codes and accelerator_ids cannot be both empty.";
  }
}

// Test that metadata with empty |replacement_codes| and grouped AcceleratorIds
// should have the same modifiers.
TEST_F(KeyboardShortcutViewerMetadataTest,
       CheckModifiersEqualForGroupedAcceleratorIdsWithEmptyReplacementCodes) {
  for (const auto& ksv_to_ids : ksv_item_to_ids_list_) {
    // This test only checks metadata with empty |replacement_codes| and grouped
    // |accelerator_ids|.
    if (!ksv_to_ids.ksv_item.replacement_codes.empty() ||
        ksv_to_ids.accelerator_ids.size() <= 1)
      continue;

    int modifiers = -1;
    for (const auto& accelerator_id : ksv_to_ids.accelerator_ids) {
      if (modifiers == -1) {
        modifiers = accelerator_id.modifiers;
      } else {
        EXPECT_EQ(modifiers, accelerator_id.modifiers)
            << "Grouped accelerator_ids with empty replacement_codes should "
               "have same modifiers: "
            << AcceleratorIdsToString(ksv_to_ids.accelerator_ids);
      }
    }
  }
}

}  // namespace ksv
