// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_CHROMEOS_KSV_MODELS_KEYBOARD_SHORTCUT_ITEM_H_
#define UI_CHROMEOS_KSV_MODELS_KEYBOARD_SHORTCUT_ITEM_H_

#include <vector>

#include "base/macros.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace ksv {

enum CATEGORY {
  kPopular = 0,
  kTabAndWindow,
  kPageAndBrowser,
  kSystemAndDisplay,
  kTextEditing,
  kAccesibility,
};

// Metadata about a keyboard shortcut. Each accelerator has a metadata with a
// CATEGORY enum and two strings to describe its categories, description, and
// shortcut. For example, a shortcut with description string of "Go to previous
// page in your browsing history" has shortcut string of "Alt + left arrow". The
// category CATEGORY::kTabAndWindow.
// To make the replacement parts highlighted in the UI, in this case, the I18n
// string of the shortcut is "<ph name="modifier">$1<ex>ALT</ex></ph> +
// <ph name="key">$2<ex>V</ex></ph>". The l10n_util::GetStringFUTF16() will
// return the offsets of the replacements, which are used to generate style
// ranges to insert bubbles for the modifiers and key. The first placeholder
// (modifier) will be replaced by text "Alt" for search. The second placeholder
// (key) will be replaced by text "left arrow" so that users can search by
// "left" and/or "arrow". But the UI representation of the key is an icon of
// "left arrow".
struct KeyboardShortcutItem {
  KeyboardShortcutItem(std::vector<CATEGORY> categories,
                       int description_id,
                       int shortcut_id,
                       std::vector<ui::KeyboardCode> replacement_ids);
  explicit KeyboardShortcutItem(const KeyboardShortcutItem& other);
  ~KeyboardShortcutItem();

  // The categories this shortcut belongs to.
  const std::vector<CATEGORY> categories;

  // The |description_id| associated string describes what the keyboard shortcut
  // can do.
  int description_id;

  // The |shortcut_id| associated string describes what the shortcut keys are.
  int shortcut_id;

  // |replacement_codes| contains the values of ui::KeyboardCode.
  // The |shortcut_id| associated string is I18n, it could be partially replaced
  // by texts for modifiers and key. The representation of a key could be a text
  // or an icon. The corrensponding texts or icon will be determined by the
  // values of ui::KeyboardCode.
  const std::vector<ui::KeyboardCode> replacement_codes;
};

}  // namespace ksv

#endif  // UI_CHROMEOS_KSV_MODELS_KEYBOARD_SHORTCUT_ITEM_H_
