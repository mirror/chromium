// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/chromeos/ksv/keyboard_shortcut_item.h"

#include <tuple>

#include "base/logging.h"

namespace ksv {

bool AcceleratorId::operator<(const AcceleratorId& other) const {
  return std::tie(keycode, modifiers) <
         std::tie(other.keycode, other.modifiers);
}

KeyboardShortcutItem::KeyboardShortcutItem() = default;

KeyboardShortcutItem::KeyboardShortcutItem(const KeyboardShortcutItem& other) =
    default;

KeyboardShortcutItem::~KeyboardShortcutItem() = default;

}  // namespace ksv
