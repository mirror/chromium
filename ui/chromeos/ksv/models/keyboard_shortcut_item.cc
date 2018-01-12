// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/chromeos/ksv/models/keyboard_shortcut_item.h"

namespace ksv {

KeyboardShortcutItem::KeyboardShortcutItem(
    std::vector<CATEGORY> categories,
    int description_id,
    int shortcut_id,
    std::vector<ui::KeyboardCode> replacement_codes)
    : categories(categories),
      description_id(description_id),
      shortcut_id(shortcut_id),
      replacement_codes(replacement_codes) {}

KeyboardShortcutItem::KeyboardShortcutItem(const KeyboardShortcutItem& other) =
    default;

KeyboardShortcutItem::~KeyboardShortcutItem() = default;

}  // namespace ksv
