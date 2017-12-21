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

struct ModifierToVkey {
  ui::EventFlags modifier;
  ui::KeyboardCode vkey;
};

const ModifierToVkey kModifierToVkey[] = {
    {ui::EF_CONTROL_DOWN, ui::VKEY_CONTROL},
    {ui::EF_ALT_DOWN, ui::VKEY_ALTGR},
    {ui::EF_SHIFT_DOWN, ui::VKEY_SHIFT},
    {ui::EF_COMMAND_DOWN, ui::VKEY_COMMAND}};
const size_t kModifierToVkeyLength = arraysize(kModifierToVkey);

struct KeyboardShortcutItemInfoToActions {
  KSVItemInfo shortcut_info;
  std::vector<ash::AcceleratorAction> actions;
};

const KeyboardShortcutItemInfoToActions kKeyboardShortcutItemInfoToActions[] = {
    { // KeyboardShortcutItemInfo
     {// |category_ids|
      {IDS_SHORTCUT_VIEWER_CATEGORY_POPULAR,
       IDS_SHORTCUT_VIEWER_CATEGORY_SYSTEM},
      // |description_id|
      IDS_SHORTCUT_VIEWER_DESCRIPTION_LOCK_SCREEN,
      // |shortcut_id|
      IDS_SHORTCUT_VIEWER_SHORTCUT_ONE_MODIFIER_ONE_KEY,
      // |replacement_ids| and |replacement_types| will be dynamically generated
      // in MakeAshKeyboardShortcutViewerMetadata().
      {},
      {}},
     // ash::AcceleratorAction
     {ash::LOCK_SCREEN}},

    // Some accelerators are grouped into one metadata for the
    // KeyboardShortcutViewer due to the similarity. E.g. the shortcut for
    // "Change screen resolution" is that "Ctrl + Shift and + or -", which
    // groups two accelerators.
    // TODO(wutao): Full list will be implemented.
    { // KeyboardShortcutItemInfo
     {// |category_ids|
      {IDS_SHORTCUT_VIEWER_CATEGORY_SYSTEM},
      // |description_id|
      IDS_SHORTCUT_VIEWER_DESCRIPTION_CHANGE_SCREEN_RESOLUTION,
      // |shortcut_id|
      IDS_SHORTCUT_VIEWER_SHORTCUT_CHANGE_SCREEN_RESOLUTION,
      // |replacement_ids|
      {ui::VKEY_CONTROL, ui::VKEY_SHIFT},
      // |replacement_types|
      {KSVReplacementType::kText, KSVReplacementType::kText}},
     // ash::AcceleratorAction
     {ash::SCALE_UI_UP, ash::SCALE_UI_DOWN}},

    // There are extra shortcuts do not have conrresponding accelerators at
    // both ash and chrome sides. E.g. the "Open the link in a new tab in the
    // background" with shortcut of "Press Ctrl and click a link".
    // TODO(wutao): Full list will be implemented.
    { // KeyboardShortcutItemInfo
     {// |category_ids|
      {IDS_SHORTCUT_VIEWER_CATEGORY_BROWSER},
      // |description_id|
      IDS_SHORTCUT_VIEWER_DESCRIPTION_OPEN_LINK_IN_NEW_TAB_BACKGROUND,
      // |shortcut_id|
      IDS_SHORTCUT_VIEWER_SHORTCUT_OPEN_LINK_IN_NEW_TAB_BACKGROUND,
      // |replacement_ids|
      {ui::VKEY_CONTROL},
      // |replacement_types|
      {KSVReplacementType::kText}},
     // ash::AcceleratorAction
     {}}};
const size_t kKeyboardShortcutItemInfoToActionsLength =
    arraysize(kKeyboardShortcutItemInfoToActions);

}  // namespace

namespace ash {

KSVItemInfoPtrList MakeAshKeyboardShortcutViewerMetadata() {
  KSVItemInfoPtrList shortcuts_info;

  std::map<ash::AcceleratorAction, ash::AcceleratorData>
      action_to_accelerator_data;
  for (size_t i = 0; i < ash::kAcceleratorDataLength; ++i) {
    const ash::AcceleratorData& accel_data = ash::kAcceleratorData[i];
    action_to_accelerator_data[accel_data.action] = accel_data;
  }
  std::map<ui::EventFlags, ui::KeyboardCode> modifier_to_vkey;
  for (size_t i = 0; i < kModifierToVkeyLength; ++i) {
    const ModifierToVkey& entry = kModifierToVkey[i];
    modifier_to_vkey[entry.modifier] = entry.vkey;
  }

  // Generate shortcut metadata for each entry in
  // kKeyboardShortcutItemInfoToActions.
  for (size_t i = 0; i < kKeyboardShortcutItemInfoToActionsLength; ++i) {
    const KeyboardShortcutItemInfoToActions& entry =
        kKeyboardShortcutItemInfoToActions[i];

    // If the actions size equals 1, we can dynamically generate replacement
    // info.
    if (entry.actions.size() == 1) {
      const ash::AcceleratorAction& action = entry.actions[0];
      auto iter = action_to_accelerator_data.find(action);
      DCHECK(iter != action_to_accelerator_data.end());
      const ash::AcceleratorData& accel_data = iter->second;
      KSVItemInfo info = entry.shortcut_info;
      // Insert shortcut replacements by the order of CTLR, ALT, SHIFT, SEARCH,
      // and then key.
      for (auto modifier : {ui::EF_CONTROL_DOWN, ui::EF_ALT_DOWN,
                            ui::EF_SHIFT_DOWN, ui::EF_COMMAND_DOWN}) {
        if (accel_data.modifiers & modifier) {
          auto iter = modifier_to_vkey.find(modifier);
          DCHECK(iter != modifier_to_vkey.end());
          info.replacement_ids.emplace_back(iter->second);
          info.replacement_types.emplace_back(KSVReplacementType::kText);
        }
      }
      info.replacement_ids.emplace_back(accel_data.keycode);
      info.replacement_types.emplace_back(KSVReplacementType::kText);
      shortcuts_info.emplace_back(KSVItemInfo::New(info));
    } else {
      shortcuts_info.emplace_back(KSVItemInfo::New(entry.shortcut_info));
    }
  }

  return shortcuts_info;
}

}  // namespace ash
