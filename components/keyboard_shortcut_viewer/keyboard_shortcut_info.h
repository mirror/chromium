// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_SHORTCUT_VIEWER_KEYBOARD_SHORTCUT_INFO_H_
#define COMPONENTS_KEYBOARD_SHORTCUT_VIEWER_KEYBOARD_SHORTCUT_INFO_H_

#include <vector>

#include "components/keyboard_shortcut_viewer/keyboard_shortcut_viewer_export.h"
#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace keyboard_shortcut_viewer {

// Shortcut string can be replaced by different replacements with modifiers,
// keys, and icon fonts.
enum ShortcutReplacementType {
  // ui modifier
  MODIFIER,
  // ui vkey
  VKEY,
  // icon font
  ICON
};

// Identify the type for each replacement in the shortcut string. In the
// keyboard shortcut view, the |replacement_id| will be converted to
// corresponding strings or icon fonts by the type.
struct ShortcutReplacementInfo {
  ShortcutReplacementInfo();
  ShortcutReplacementInfo(int replacement_id,
                          ShortcutReplacementType replacement_type);
  ~ShortcutReplacementInfo();

  bool operator==(const ShortcutReplacementInfo& other) const;

  // |replacement_id| could be ui modifier, ui vkey, and icon font id.
  int replacement_id;
  ShortcutReplacementType replacement_type;
};

// Provide metadata for each accelerator item in KeyboardShortcutViewer.
// Each accelerator has three basic string info: category, description (what to
// achieve), and shortcut (what to do).
struct KeyboardShortcutItemInfo {
  KeyboardShortcutItemInfo();
  KeyboardShortcutItemInfo(std::vector<int> categories,
                           int description,
                           int shortcut);
  KeyboardShortcutItemInfo(
      std::vector<int> categories,
      int description,
      int shortcut,
      std::vector<ShortcutReplacementInfo> shortcut_replacements_info);
  KeyboardShortcutItemInfo(const KeyboardShortcutItemInfo& other);
  ~KeyboardShortcutItemInfo();

  bool operator==(const KeyboardShortcutItemInfo& other) const;

  std::vector<int> categories;
  int description;
  // The |shortcut| string could be partially replaced by text for modifier,
  // key, and icon font at the placeholders, which is held in
  // |shortcut_replacements_info|.
  int shortcut;
  std::vector<ShortcutReplacementInfo> shortcut_replacements_info;
};

// Convert a |MODIFIER| replacement to corresponding I18n message.
struct ModifierToI18nMessage {
  const ui::EventFlags modifier;
  int message;
};

// Convert a |VKEY| replacement to corresponding I18n message.
struct VKeyToI18nMessage {
  const ui::KeyboardCode vkey;
  int message;
};

// TODO(wutao): Implement this. This code is here to show how we will use this
// to locate the icon font id.
// struct VKeyToIconFont {
//   const ui::KeyboardCode vkey;
//   int message;
// };

KEYBOARD_SHORTCUT_VIEWER_EXPORT extern const ModifierToI18nMessage
    kModifierToI18nMessage[];
KEYBOARD_SHORTCUT_VIEWER_EXPORT extern const size_t
    kModifierToI18nMessageLength;
KEYBOARD_SHORTCUT_VIEWER_EXPORT extern const VKeyToI18nMessage
    kVKeyToI18nMessage[];
KEYBOARD_SHORTCUT_VIEWER_EXPORT extern const size_t kVKeyToI18nMessageLength;

}  // namespace keyboard_shortcut_viewer

#endif  // COMPONENTS_KEYBOARD_SHORTCUT_VIEWER_KEYBOARD_SHORTCUT_INFO_H_
