// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accelerators/keyboard_shortcut_viewer_metadata.h"

#include "base/macros.h"
#include "components/strings/grit/components_strings.h"

// TODO(wutao): These are some examples how we can organize the metadata. Full
// mappings will be implemented.

namespace {

using KSVItemInfo = keyboard_shortcut_viewer::mojom::KeyboardShortcutItemInfo;
using KSVReplacementType =
    keyboard_shortcut_viewer::mojom::ShortcutReplacementType;

// Some accelerators are grouped into one metadata for the
// KeyboardShortcutViewer due to the similarity. E.g. the shortcut for "Change
// screen resolution" is that "Ctrl + Shift and + or -", which groups two
// accelerators.
// TODO(wutao): This is an example. Full list will be implemented.
enum GroupedAction {
  CHANGE_SCREEN_RESOLUTION,
};

// This enum includes extra shortcuts do not have conrresponding accelerators at
// both ash and chrome sides. E.g. the "Open the link in a new tab in the
// background" with shortcut of "Press Ctrl and click a link".
// TODO(wutao): This is an example. Full list will be implemented.
enum ExtraAction {
  OPEN_LINK_IN_NEW_TAB_BACKGROUND,
};

struct ActionToGroupedAction {
  ash::AcceleratorAction action;
  GroupedAction grouped_action;
};

// Ash accelerator action to metadata.
struct ActionToKeyboardShortcutItemInfo {
  ash::AcceleratorAction action;
  KSVItemInfo shortcut_info;
};

// Grouped accelerator action to metadata.
struct GroupedActionToKeyboardShortcutItemInfo {
  GroupedAction grouped_action;
  KSVItemInfo shortcut_info;
};

// Extra accelerator action to metadata.
struct ExtraActionToKeyboardShortcutItemInfo {
  ExtraAction extra_action;
  KSVItemInfo shortcut_info;
};

const ActionToKeyboardShortcutItemInfo kActionToKeyboardShortcutItemInfo[] = {
    {ash::LOCK_SCREEN,
     // KeyboardShortcutItemInfo
     {// |category_ids|
      {IDS_KEYBOARD_SHORTCUT_VIEWER_CATEGORY_POPULAR,
       IDS_KEYBOARD_SHORTCUT_VIEWER_CATEGORY_SYSTEM},
      // |description_id|
      IDS_KEYBOARD_SHORTCUT_VIEWER_DESCRIPTION_LOCK_SCREEN,
      // |shortcut_id|
      IDS_KEYBOARD_SHORTCUT_VIEWER_SHORTCUT_ONE_MODIFIER_ONE_KEY,
      // |replacement_ids| and |replacement_types| will be dynamically generated
      // in MakeAshKeyboardShortcutViewerMetadata().
      {},
      {}}}};
const size_t kActionToKeyboardShortcutItemInfoLength =
    arraysize(kActionToKeyboardShortcutItemInfo);

// TODO(wutao): This is an example how it looks like for a grouped_action. It
// will be used to check every action shuold have one corresponding keyboard
// shortcut metadata. The full list will be implemented.
// const ActionToGroupedAction kActionToGroupedAction[] = {
//    {ash::SCALE_UI_UP, CHANGE_SCREEN_RESOLUTION},
//    {ash::SCALE_UI_DOWN, CHANGE_SCREEN_RESOLUTION}};
// const size_t kActionToGroupedActionLength =
// arraysize(kActionToGroupedAction);

const GroupedActionToKeyboardShortcutItemInfo
    kGroupedActionToKeyboardShortcutItemInfo[] = {
        {CHANGE_SCREEN_RESOLUTION,
         // KeyboardShortcutItemInfo
         {// |category_ids|
          {IDS_KEYBOARD_SHORTCUT_VIEWER_CATEGORY_SYSTEM},
          // |description_id|
          IDS_KEYBOARD_SHORTCUT_VIEWER_DESCRIPTION_CHANGE_SCREEN_RESOLUTION,
          // |shortcut_id|
          IDS_KEYBOARD_SHORTCUT_VIEWER_SHORTCUT_CHANGE_SCREEN_RESOLUTION,
          // |replacement_ids|
          {ui::EF_CONTROL_DOWN, ui::EF_SHIFT_DOWN},
          // |replacement_types|
          {KSVReplacementType::MODIFIER, KSVReplacementType::MODIFIER}}}};
const size_t kGroupedActionToKeyboardShortcutItemInfoLength =
    arraysize(kGroupedActionToKeyboardShortcutItemInfo);

const ExtraActionToKeyboardShortcutItemInfo
    kExtraActionToKeyboardShortcutItemInfo[] = {
        {OPEN_LINK_IN_NEW_TAB_BACKGROUND,
         // KeyboardShortcutItemInfo
         {// |category_ids|
          {IDS_KEYBOARD_SHORTCUT_VIEWER_CATEGORY_BROWSER},
          // |description_id|
          IDS_KEYBOARD_SHORTCUT_VIEWER_DESCRIPTION_OPEN_LINK_IN_NEW_TAB_BACKGROUND,
          // |shortcut_id|
          IDS_KEYBOARD_SHORTCUT_VIEWER_SHORTCUT_OPEN_LINK_IN_NEW_TAB_BACKGROUND,
          // |replacement_ids|
          {ui::EF_CONTROL_DOWN},
          // |replacement_types|
          {KSVReplacementType::MODIFIER}}}};
const size_t kExtraActionToKeyboardShortcutItemInfoLength =
    arraysize(kExtraActionToKeyboardShortcutItemInfo);

}  // namespace

namespace ash {

KSVItemInfoPtrList MakeAshKeyboardShortcutViewerMetadata() {
  KSVItemInfoPtrList shortcuts_info;

  std::map<ash::AcceleratorAction, KSVItemInfo> actions_shortcut_info;
  for (size_t i = 0; i < kActionToKeyboardShortcutItemInfoLength; ++i) {
    const ActionToKeyboardShortcutItemInfo& entry =
        kActionToKeyboardShortcutItemInfo[i];
    actions_shortcut_info[entry.action] = entry.shortcut_info;
  }

  // Generate shortcut metadata for each action.
  for (size_t i = 0; i < ash::kAcceleratorDataLength; ++i) {
    const ash::AcceleratorData& accel_data = ash::kAcceleratorData[i];
    ash::AcceleratorAction action = accel_data.action;
    // TODO(wutao): Currently skip accelerators without metadata to demonstrate
    // how this code can work. In the future cl, every accelerator should have
    // one metadata and verified by DCHECK and test.
    if (actions_shortcut_info.find(action) == actions_shortcut_info.end()) {
      // TODO(wutao): Will have DCHECK here, if an action has no metadata, then
      // this action should have a corresponding grouped_action.
      continue;
    }

    KSVItemInfo info;
    info = actions_shortcut_info[action];
    // Insert shortcut replacements by the order of CTLR, ALT, SHIFT, SEARCH,
    // and then key.
    for (auto modifier : {ui::EF_CONTROL_DOWN, ui::EF_ALT_DOWN,
                          ui::EF_SHIFT_DOWN, ui::EF_COMMAND_DOWN}) {
      if (accel_data.modifiers & modifier) {
        info.replacement_ids.push_back(modifier);
        info.replacement_types.push_back(KSVReplacementType::MODIFIER);
      }
    }
    // If key in this list, then use icon.
    info.replacement_ids.push_back(accel_data.keycode);
    info.replacement_types.push_back(KSVReplacementType::VKEY);
    shortcuts_info.push_back(KSVItemInfo::New(info));
  }

  // Generate shortcut metadata for each grouped_action.
  for (size_t i = 0; i < kGroupedActionToKeyboardShortcutItemInfoLength; ++i) {
    const GroupedActionToKeyboardShortcutItemInfo& entry =
        kGroupedActionToKeyboardShortcutItemInfo[i];
    KSVItemInfo info = entry.shortcut_info;
    shortcuts_info.push_back(KSVItemInfo::New(info));
  }

  // Generate shortcut metadata for each extra_action.
  for (size_t i = 0; i < kExtraActionToKeyboardShortcutItemInfoLength; ++i) {
    const ExtraActionToKeyboardShortcutItemInfo& entry =
        kExtraActionToKeyboardShortcutItemInfo[i];
    KSVItemInfo info = entry.shortcut_info;
    shortcuts_info.push_back(KSVItemInfo::New(info));
  }
  return shortcuts_info;
}

}  // namespace ash
