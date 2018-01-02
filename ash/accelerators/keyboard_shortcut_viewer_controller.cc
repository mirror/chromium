// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accelerators/keyboard_shortcut_viewer_controller.h"

#include "ash/accelerators/keyboard_shortcut_viewer_metadata.h"
#include "base/bind.h"

namespace {

// Get the keyboard codes for modifiers.
ui::KeyboardCode GetModifierKeyCode(ui::EventFlags modifier) {
  switch (modifier) {
    case ui::EF_CONTROL_DOWN:
      return ui::VKEY_CONTROL;
    case ui::EF_ALT_DOWN:
      return ui::VKEY_ALTGR;
    case ui::EF_SHIFT_DOWN:
      return ui::VKEY_SHIFT;
    case ui::EF_COMMAND_DOWN:
      return ui::VKEY_COMMAND;
    default:
      NOTREACHED();
      return ui::VKEY_UNKNOWN;
  }
}

// TODO(wutao): this list is not fullly implemented yet.
bool ShouldPaintKeyAsIcon(ui::KeyboardCode vkey) {
  switch (vkey) {
    case ui::VKEY_BRIGHTNESS_DOWN:
    case ui::VKEY_BRIGHTNESS_UP:
    case ui::VKEY_BROWSER_BACK:
    case ui::VKEY_BROWSER_FORWARD:
    case ui::VKEY_BROWSER_REFRESH:
    // Overview mode key
    case ui::VKEY_MEDIA_LAUNCH_APP1:
    // Fullscreen
    case ui::VKEY_MEDIA_LAUNCH_APP2:
    case ui::VKEY_VOLUME_DOWN:
    case ui::VKEY_VOLUME_MUTE:
    case ui::VKEY_VOLUME_UP:
    case ui::VKEY_DOWN:
    case ui::VKEY_LEFT:
    case ui::VKEY_RIGHT:
    case ui::VKEY_UP:
      return true;
    default:
      return false;
  }
}

}  // namespace

namespace ash {

KeyboardShortcutViewerController::KeyboardShortcutViewerController()
    : weak_ptr_factory_(this) {}

KeyboardShortcutViewerController::~KeyboardShortcutViewerController() = default;

void KeyboardShortcutViewerController::BindRequest(
    keyboard_shortcut_viewer::KSVControllerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void KeyboardShortcutViewerController::SetClient(
    keyboard_shortcut_viewer::KSVClientPtr client) {
  client_ = std::move(client);

  if (client_) {
    client_.set_connection_error_handler(
        base::BindOnce(&KeyboardShortcutViewerController::SetClient,
                       weak_ptr_factory_.GetWeakPtr(), nullptr));
  }
}

// keyboard_shortcut_viewer::mojom::KeyboardShortcutViewerController:
void KeyboardShortcutViewerController::OpenKeyboardShortcutViewer() {
  if (client_) {
    client_->GetKeyboardShortcutsInfo(base::BindOnce(
        &KeyboardShortcutViewerController::ShowKeyboardShortcutViewer,
        weak_ptr_factory_.GetWeakPtr()));
  } else {
    // Add an empty client shortcut list.
    ShowKeyboardShortcutViewer(keyboard_shortcut_viewer::KSVItemInfoPtrList());
  }
}

void KeyboardShortcutViewerController::ShowKeyboardShortcutViewer(
    keyboard_shortcut_viewer::KSVItemInfoPtrList client_shortcuts_info) {
  keyboard_shortcut_viewer::KSVItemInfoPtrList shortcuts_info =
      GetAshShortcutsMetadata();
  shortcuts_info.insert(shortcuts_info.end(),
                        std::make_move_iterator(client_shortcuts_info.begin()),
                        std::make_move_iterator(client_shortcuts_info.end()));

  // TODO(wutao): To set the |shortcuts_info| to the model of
  // KeyboardShortcutView.
  NOTIMPLEMENTED();
}

keyboard_shortcut_viewer::KSVItemInfoPtrList
KeyboardShortcutViewerController::GetAshShortcutsMetadata() {
  keyboard_shortcut_viewer::KSVItemInfoPtrList shortcuts_info;

  if (ash_shortcuts_metadata_.empty()) {
    std::map<AcceleratorAction, std::vector<AcceleratorData>>
        action_to_accelerator_data;
    for (size_t i = 0; i < kAcceleratorDataLength; ++i) {
      const AcceleratorData& accel_data = kAcceleratorData[i];
      action_to_accelerator_data[accel_data.action].emplace_back(accel_data);
    }

    // Generate shortcut metadata for each entry in
    // kKeyboardShortcutItemInfoToActions.
    for (size_t i = 0; i < kKSVItemInfoToActionsLength; ++i) {
      const KSVItemInfoToActions& entry = kKSVItemInfoToActions[i];

      // If the |replacement_ids| is empty, we need to dynamically generate the
      // replacement ids and types.
      if (entry.shortcut_info.replacement_ids.empty()) {
        DCHECK(!entry.actions.empty());
        // Only use the first action because the modifiers are the same even if
        // it is a grouped actions. If the modifiers are not the same, we do not
        // know how to generate the replacements, in which we would like to
        // provide replacements infomation directly.
        const AcceleratorAction& action = entry.actions[0];
        auto iter = action_to_accelerator_data.find(action);
        DCHECK(iter != action_to_accelerator_data.end());

        // If one AcceleratorAction have many AcceleratorData, we do not know
        // how to generate the replacements. Therefore, replacement information
        // would already be provided in the |kKSVItemInfoToActions| for this
        // entry.
        DCHECK(iter->second.size() == 1);
        const AcceleratorData& accel_data = iter->second[0];
        keyboard_shortcut_viewer::KSVItemInfo info = entry.shortcut_info;
        // Insert shortcut replacements by the order of CTLR, ALT, SHIFT,
        // SEARCH, and then key, to be consistent with how we describe it in the
        // |shortcut_id| associated string.
        for (auto modifier : {ui::EF_CONTROL_DOWN, ui::EF_ALT_DOWN,
                              ui::EF_SHIFT_DOWN, ui::EF_COMMAND_DOWN}) {
          if (accel_data.modifiers & modifier) {
            info.replacement_ids.emplace_back(GetModifierKeyCode(modifier));
            info.replacement_types.emplace_back(
                keyboard_shortcut_viewer::KSVReplacementType::kText);
          }
        }
        // For non grouped actions, we need to replace the key as well.
        if (entry.actions.size() == 1) {
          info.replacement_ids.emplace_back(accel_data.keycode);
          info.replacement_types.emplace_back(
              ShouldPaintKeyAsIcon(accel_data.keycode)
                  ? keyboard_shortcut_viewer::KSVReplacementType::kIcon
                  : keyboard_shortcut_viewer::KSVReplacementType::kText);
        }
        ash_shortcuts_metadata_.emplace_back(info);
      } else {
        // Replacement information were statically provided. This happens for
        // the cases when one AcceleratorAction have many AcceleratorData.
        ash_shortcuts_metadata_.emplace_back(entry.shortcut_info);
      }
    }
  }

  // Cannot cache |shortcuts_info| directly because it is a vector of
  // mojo::StructPtr, which will be moved in ShowKeyboardShortcutViewer().
  for (const auto& info : ash_shortcuts_metadata_) {
    shortcuts_info.emplace_back(
        keyboard_shortcut_viewer::KSVItemInfo::New(info));
  }
  return shortcuts_info;
}

}  // namespace ash
