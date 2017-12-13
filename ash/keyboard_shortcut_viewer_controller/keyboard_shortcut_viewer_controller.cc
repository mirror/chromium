// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/keyboard_shortcut_viewer_controller/keyboard_shortcut_viewer_controller.h"

#include "ash/accelerators/accelerator_table.h"
#include "ash/keyboard_shortcut_viewer_controller/keyboard_shortcut_data.h"
#include "components/keyboard_shortcut_viewer/keyboard_shortcut_info.h"

namespace {

std::vector<keyboard_shortcut_viewer::KeyboardShortcutItemInfo>
MakeAshShortcutsInfo() {
  std::vector<keyboard_shortcut_viewer::KeyboardShortcutItemInfo>
      shortcuts_info;

  std::map<ash::AcceleratorAction,
           keyboard_shortcut_viewer::KeyboardShortcutItemInfo>
      actions_shortcut_info;
  for (size_t i = 0; i < ash::kActionToKeyboardShortcutItemInfoLength; ++i) {
    const ash::ActionToKeyboardShortcutItemInfo& entry =
        ash::kActionToKeyboardShortcutItemInfo[i];
    actions_shortcut_info[entry.action] = entry.shortcut_info;
  }

  // Generate shortcut metadata for each action.
  for (size_t i = 0; i < ash::kAcceleratorDataLength; ++i) {
    const ash::AcceleratorData& accel_data = ash::kAcceleratorData[i];
    ash::AcceleratorAction action = accel_data.action;
    // TODO(wutao): Currently skip accelerators without metadata to demonstrate
    // how this code can work. In the future cl, every accelerator should have
    // one metadata and verified by test.
    if (actions_shortcut_info.find(action) == actions_shortcut_info.end())
      continue;

    keyboard_shortcut_viewer::KeyboardShortcutItemInfo info;
    info = actions_shortcut_info[action];
    // Insert shortcut replacements by the order of CTLR, ALT, SHIFT, SEARCH,
    // and then key.
    for (auto modifier : {ui::EF_CONTROL_DOWN, ui::EF_ALT_DOWN,
                          ui::EF_SHIFT_DOWN, ui::EF_COMMAND_DOWN}) {
      if (accel_data.modifiers & modifier) {
        info.shortcut_replacements_info.push_back(
            {modifier, keyboard_shortcut_viewer::MODIFIER});
      }
    }
    info.shortcut_replacements_info.push_back(
        {accel_data.keycode, keyboard_shortcut_viewer::VKEY});
    shortcuts_info.push_back(info);
  }

  // Generate shortcut metadata for each grouped_action.
  for (size_t i = 0; i < ash::kGroupedActionToKeyboardShortcutItemInfoLength;
       ++i) {
    const ash::GroupedActionToKeyboardShortcutItemInfo& entry =
        ash::kGroupedActionToKeyboardShortcutItemInfo[i];
    keyboard_shortcut_viewer::KeyboardShortcutItemInfo info =
        entry.shortcut_info;
    shortcuts_info.push_back(info);
  }
  return shortcuts_info;
}

}  // namespace

namespace ash {

KeyboardShortcutViewerController::KeyboardShortcutViewerController() = default;

KeyboardShortcutViewerController::~KeyboardShortcutViewerController() = default;

void KeyboardShortcutViewerController::BindRequest(
    mojom::KeyboardShortcutViewerControllerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void KeyboardShortcutViewerController::SetClient(
    mojom::KeyboardShortcutViewerControllerClientPtr client) {
  client_ = std::move(client);
}

// mojom::KeyboardShortcutViewerController:
void KeyboardShortcutViewerController::UpdateKeyboardShortcutsInfo(
    const KeyboardShortcutItemInfoList& client_shortcuts_info) {
  if (ash_shortcuts_info_.empty())
    ash_shortcuts_info_ = MakeAshShortcutsInfo();
  KeyboardShortcutItemInfoList shortcuts_info;
  shortcuts_info.insert(shortcuts_info.end(), ash_shortcuts_info_.begin(),
                        ash_shortcuts_info_.end());
  shortcuts_info.insert(shortcuts_info.end(), client_shortcuts_info.begin(),
                        client_shortcuts_info.end());
  ShowKeyboardShortcutViewer(shortcuts_info);
}

void KeyboardShortcutViewerController::GetClientKeyboardShortcutsInfo() {
  if (client_)
    client_->GetKeyboardShortcutsInfo();
}

void KeyboardShortcutViewerController::OpenKeyboardShortcutViewer() {
  // Defer to the client to call UpdateKeyboardShortcutsInfo() to push clide
  // side shortcuts info and ShowKeyboardShortcutViewer().
  GetClientKeyboardShortcutsInfo();
}

void KeyboardShortcutViewerController::ShowKeyboardShortcutViewer(
    const KeyboardShortcutItemInfoList& shortcuts_info) {
  NOTIMPLEMENTED();
}

}  // namespace ash
