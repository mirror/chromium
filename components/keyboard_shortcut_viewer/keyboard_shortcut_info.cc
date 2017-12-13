// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_shortcut_viewer/keyboard_shortcut_info.h"

#include <set>

#include "base/macros.h"
#include "components/strings/grit/components_strings.h"

// TODO(wutao): This is an example how we can organize the metadata. Full
// mappings will be implemented.

namespace keyboard_shortcut_viewer {

const ModifierToI18nMessage kModifierToI18nMessage[] = {
    {ui::EF_CONTROL_DOWN, IDS_KEYBOARD_SHORTCUT_VIEWER_MODIFIER_CTRL},
    {ui::EF_ALT_DOWN, IDS_KEYBOARD_SHORTCUT_VIEWER_MODIFIER_ALT},
    {ui::EF_SHIFT_DOWN, IDS_KEYBOARD_SHORTCUT_VIEWER_MODIFIER_SHIFT},
    {ui::EF_COMMAND_DOWN, IDS_KEYBOARD_SHORTCUT_VIEWER_MODIFIER_SEARCH}};
const size_t kModifierToI18nMessageLength = arraysize(kModifierToI18nMessage);

// Most of the KeyboardCode can be converted to corresponding characters.
// However, there are special mappings.
const VKeyToI18nMessage kVKeyToI18nMessage[] = {
    {ui::VKEY_OEM_PLUS, IDS_KEYBOARD_SHORTCUT_VIEWER_VKEY_OEM_PLUS},
    {ui::VKEY_OEM_MINUS, IDS_KEYBOARD_SHORTCUT_VIEWER_VKEY_OEM_MINUS},
};
const size_t kVKeyToI18nMessageLength = arraysize(kVKeyToI18nMessage);

// TODO(wutao): Implement this. This code is here to show how we will use this
// to locate the icon font id.
// VKeyToIconFont kVKeyToIconFonts[] = {
// };

ShortcutReplacementInfo::ShortcutReplacementInfo() = default;
ShortcutReplacementInfo::ShortcutReplacementInfo(
    int replacement_id,
    ShortcutReplacementType replacement_type)
    : replacement_id(replacement_id), replacement_type(replacement_type) {}
ShortcutReplacementInfo::~ShortcutReplacementInfo() = default;
bool ShortcutReplacementInfo::operator==(
    const ShortcutReplacementInfo& other) const {
  return (replacement_id == other.replacement_id) &&
         (replacement_type == other.replacement_type);
}

KeyboardShortcutItemInfo::KeyboardShortcutItemInfo() = default;
KeyboardShortcutItemInfo::KeyboardShortcutItemInfo(std::vector<int> categories,
                                                   int description,
                                                   int shortcut)
    : categories(categories), description(description), shortcut(shortcut) {}
KeyboardShortcutItemInfo::KeyboardShortcutItemInfo(
    std::vector<int> categories,
    int description,
    int shortcut,
    std::vector<ShortcutReplacementInfo> shortcut_replacements_info)
    : categories(categories),
      description(description),
      shortcut(shortcut),
      shortcut_replacements_info(shortcut_replacements_info) {}
KeyboardShortcutItemInfo::KeyboardShortcutItemInfo(
    const KeyboardShortcutItemInfo& other) = default;
KeyboardShortcutItemInfo::~KeyboardShortcutItemInfo() = default;

bool KeyboardShortcutItemInfo::operator==(
    const KeyboardShortcutItemInfo& other) const {
  const std::set<int> category_set(categories.begin(), categories.end());
  const std::set<int> other_category_set(other.categories.begin(),
                                         other.categories.end());
  return (category_set == other_category_set) &&
         (description == other.description) && (shortcut == other.shortcut) &&
         (shortcut_replacements_info == other.shortcut_replacements_info);
}

}  // namespace keyboard_shortcut_viewer
