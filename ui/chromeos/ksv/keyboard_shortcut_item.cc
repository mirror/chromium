// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/chromeos/ksv/keyboard_shortcut_item.h"

#include "base/logging.h"

namespace ksv {

bool AcceleratorId::operator<(const AcceleratorId& other) const {
  if (keycode != other.keycode)
    return keycode < other.keycode;
  return modifiers < other.modifiers;
}

KeyboardShortcutItem::KeyboardShortcutItem(
    std::vector<Category> categories,
    int description_string_id,
    int shortcut_string_id,
    std::vector<AcceleratorId> accelerator_ids,
    std::vector<ui::KeyboardCode> replacement_codes)
    : categories(categories),
      description_string_id(description_string_id),
      shortcut_string_id(shortcut_string_id),
      accelerator_ids(accelerator_ids),
      replacement_codes(replacement_codes) {
  DCHECK(!categories.empty());
  DCHECK(!accelerator_ids.empty() || !replacement_codes.empty());
}

KeyboardShortcutItem::KeyboardShortcutItem(const KeyboardShortcutItem& other) =
    default;

KeyboardShortcutItem::~KeyboardShortcutItem() = default;

}  // namespace ksv
