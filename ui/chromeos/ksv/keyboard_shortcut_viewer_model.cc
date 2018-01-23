// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/chromeos/ksv/keyboard_shortcut_viewer_model.h"

#include "ui/chromeos/ksv/keyboard_shortcut_item.h"
#include "ui/chromeos/ksv/keyboard_shortcut_viewer_metadata.h"
#include "ui/chromeos/ksv/keyboard_shortcut_viewer_model_observer.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace keyboard_shortcut_viewer {

namespace {

// Get the keyboard codes for modifiers.
ui::KeyboardCode GetModifierKeyCode(ui::EventFlags modifier) {
  switch (modifier) {
    case ui::EF_CONTROL_DOWN:
      return ui::VKEY_CONTROL;
    case ui::EF_ALT_DOWN:
      return ui::VKEY_LMENU;
    case ui::EF_SHIFT_DOWN:
      return ui::VKEY_SHIFT;
    case ui::EF_COMMAND_DOWN:
      return ui::VKEY_COMMAND;
    default:
      NOTREACHED();
      return ui::VKEY_UNKNOWN;
  }
}

}  // namespace

KeyboardShortcutViewerModel::KeyboardShortcutViewerModel() = default;

KeyboardShortcutViewerModel::~KeyboardShortcutViewerModel() = default;

// This should be only called once because we do not support udpating the
// shortcut items.
void KeyboardShortcutViewerModel::MakeKeyboardShortcutItems() {
  // Generate KeyboardShortcutItem and populate |shortcut_key_codes| if
  // neccessary.
  for (auto& item : GetKeyboardShortcutItemList()) {
    // If the |shortcut_key_codes| is empty, we need to dynamically populate the
    // keycodes for replacements.
    if (item.shortcut_key_codes.empty()) {
      DCHECK(!item.accelerator_ids.empty());
      // Only use the first |accelerator_id| because the modifiers are the same
      // even if it is a grouped accelerators.
      const AcceleratorId& accelerator_id = item.accelerator_ids[0];
      KeyboardShortcutItem entry(item);
      // Insert |shortcut_key_codes| by the order of CTLR, ALT, SHIFT, SEARCH,
      // and then key, to be consistent with how we describe it in the
      // |shortcut_message_id| associated string template.
      for (auto modifier : {ui::EF_CONTROL_DOWN, ui::EF_ALT_DOWN,
                            ui::EF_SHIFT_DOWN, ui::EF_COMMAND_DOWN}) {
        if (accelerator_id.modifiers & modifier)
          entry.shortcut_key_codes.emplace_back(GetModifierKeyCode(modifier));
      }
      // For non grouped accelerators, we need to populate the key as well.
      if (item.accelerator_ids.size() == 1)
        entry.shortcut_key_codes.emplace_back(accelerator_id.keycode);

      shortcut_items_.emplace_back(
          std::make_unique<KeyboardShortcutItem>(entry));
    } else {
      // |shortcut_key_codes| were statically provided.
      shortcut_items_.emplace_back(
          std::make_unique<KeyboardShortcutItem>(item));
    }
  }

  for (auto& observer : observers_)
    observer.OnKeyboardShortcutViewerModelChanged();
}

void KeyboardShortcutViewerModel::AddObserver(
    KeyboardShortcutViewerModelObserver* observer) {
  observers_.AddObserver(observer);
}
void KeyboardShortcutViewerModel::RemoveObserver(
    KeyboardShortcutViewerModelObserver* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace keyboard_shortcut_viewer
