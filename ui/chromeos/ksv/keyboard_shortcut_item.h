// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_ITEM_H_
#define UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_ITEM_H_

#include <vector>

#include "base/macros.h"
#include "ui/chromeos/ksv/ksv_export.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace ksv {

// The categories the shortcut belongs to.
enum class ShortcutCategory {
  kPopular = 0,
  kTabAndWindow,
  kPageAndBrowser,
  kSystemAndDisplay,
  kTextEditing,
  kAccessibility,
};

// We had to duplicate the key and modifiers to map to an accelerator to avoid
// deps on //ash and //chrome. This is used to dynamically generate the
// KeyboardShortcutItem |replacement_codes| and in test.
struct KSV_EXPORT AcceleratorId {
  bool operator<(const AcceleratorId& other) const;

  ui::KeyboardCode keycode = ui::VKEY_UNKNOWN;
  int modifiers = 0;
};

// Metadata about a keyboard shortcut. Each accelerator has a metadata with a
// category enum and two strings to describe its categories, description, and
// shortcut. For example, a shortcut with description string of "Go to previous
// page in your browsing history" has shortcut string of "Alt + left arrow" and
// the category of Category::kTabAndWindow.
// To make the replacement parts highlighted in the UI, in this case, the I18n
// string of the shortcut is "<ph name="modifier">$1<ex>ALT</ex></ph> +
// <ph name="key">$2<ex>V</ex></ph>". The l10n_util::GetStringFUTF16() will
// return the offsets of the replacements, which are used to generate style
// ranges to insert symbols for the modifiers and key. The first placeholder
// (modifier) will be replaced by text "Alt" for search. The second placeholder
// (key) will be replaced by text "left arrow" so that users can search by
// "left" and/or "arrow". But the UI representation of the key is an icon of
// "left arrow".
struct KSV_EXPORT KeyboardShortcutItem {
  KeyboardShortcutItem();
  explicit KeyboardShortcutItem(const KeyboardShortcutItem& other);
  ~KeyboardShortcutItem();

  // The categories this shortcut belongs to.
  std::vector<ShortcutCategory> categories;

  // The |description_string_id| associated string describes what the keyboard
  // shortcut can do.
  int description_string_id;

  // The |shortcut_string_id| associated string describes what the shortcut keys
  // are.
  int shortcut_string_id;

  // Multiple accelerators can be mapped to the same ksv item.
  // |replacement_codes| could be auto-generated from |accelerator_ids| to avoid
  // duplicates.
  std::vector<AcceleratorId> accelerator_ids;

  // |replacement_codes| contains the values of ui::KeyboardCode.
  // The |shortcut_string_id| associated string is I18n, it could be partially
  // replaced by texts for modifiers and key. The representation of a key could
  // be a text or an icon. The corresponding texts or icon will be determined by
  // the values of ui::KeyboardCode.
  // For example of shortcut "Alt + left arrow", this will be
  // {ui::VKEY_LMENU, ui::VKEY_LEFT}. Note that the modifier is converted to
  // ui::KeyboardCode so that there is only one enum type to deal with.
  std::vector<ui::KeyboardCode> replacement_codes;
};

}  // namespace ksv

#endif  // UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_ITEM_H_
