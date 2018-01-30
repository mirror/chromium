// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_VIEWER_METADATA_H_
#define UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_VIEWER_METADATA_H_

#include <vector>

#include "base/containers/span.h"
#include "base/strings/string16.h"
#include "ui/chromeos/ksv/ksv_export.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace keyboard_shortcut_viewer {

struct KeyboardShortcutItem;
enum class ShortcutCategory;

// Returns a list of Ash and Chrome keyboard shortcuts metadata.
KSV_EXPORT const std::vector<KeyboardShortcutItem>&
GetKeyboardShortcutItemList();

// The order of the categories shown on the side tabs.
const base::span<const ShortcutCategory> GetCategoryDisplayOrder();

const base::string16 GetStringForCategory(ShortcutCategory category);

// TODO(https://crbug.com/803502): Get labels for non US keyboard layout.
const base::string16 GetStringForKeyboardCode(ui::KeyboardCode keycode,
                                              bool shift_down);

}  // namespace keyboard_shortcut_viewer

#endif  // UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_VIEWER_METADATA_H_
