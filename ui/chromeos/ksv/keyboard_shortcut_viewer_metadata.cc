// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/chromeos/ksv/keyboard_shortcut_viewer_metadata.h"

#include <map>

#include "base/logging.h"
#include "base/macros.h"
#include "ui/chromeos/ksv/keyboard_shortcut_item.h"
#include "ui/chromeos/strings/grit/ui_chromeos_strings.h"
#include "ui/events/event_constants.h"

namespace ksv {

namespace {

struct DescriptionStringIdToShortcutStringId {
  int descrption_string_id;
  int shortcut_string_id;
};

struct DescriptionStringIdToCategory {
  int descrption_string_id;
  ShortcutCategory category;
};

struct DescriptionStringIdToAcceleratorId {
  int descrption_string_id;
  AcceleratorId accelerator_id;
};

// TODO(wutao): These are two examples how we organize the metadata. Full
// mappings will be implemented.
constexpr DescriptionStringIdToShortcutStringId kDescriptionToShortcut[] = {
    {IDS_KSV_DESCRIPTION_LOCK_SCREEN, IDS_KSV_SHORTCUT_ONE_MODIFIER_ONE_KEY},
    {IDS_KSV_DESCRIPTION_CHANGE_SCREEN_RESOLUTION,
     IDS_KSV_SHORTCUT_CHANGE_SCREEN_RESOLUTION}};
constexpr size_t kDescriptionToShortcutLength =
    arraysize(kDescriptionToShortcut);

constexpr DescriptionStringIdToCategory kDescriptionToCategory[] = {
    {IDS_KSV_DESCRIPTION_LOCK_SCREEN, ShortcutCategory::kPopular},
    {IDS_KSV_DESCRIPTION_LOCK_SCREEN, ShortcutCategory::kSystemAndDisplay},
    {IDS_KSV_DESCRIPTION_CHANGE_SCREEN_RESOLUTION,
     ShortcutCategory::kSystemAndDisplay}};
constexpr size_t kDescriptionToCategoryLength =
    arraysize(kDescriptionToCategory);

// Some accelerators are grouped into one metadata for the
// KeyboardShortcutViewer due to the similarity. E.g. the shortcut for
// "Change screen resolution" is "Ctrl + Shift and + or -", which groups two
// accelerators.
constexpr DescriptionStringIdToAcceleratorId kDescriptionToAcceleratorId[] = {
    {IDS_KSV_DESCRIPTION_LOCK_SCREEN, {ui::VKEY_L, ui::EF_COMMAND_DOWN}},
    {IDS_KSV_DESCRIPTION_CHANGE_SCREEN_RESOLUTION,
     {ui::VKEY_OEM_MINUS, ui::EF_CONTROL_DOWN | ui::EF_SHIFT_DOWN}},
    {IDS_KSV_DESCRIPTION_CHANGE_SCREEN_RESOLUTION,
     {ui::VKEY_OEM_PLUS, ui::EF_CONTROL_DOWN | ui::EF_SHIFT_DOWN}}};
constexpr size_t kDescriptionToAcceleratorIdLength =
    arraysize(kDescriptionToAcceleratorId);

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

std::vector<std::unique_ptr<KeyboardShortcutItem>>
GetKeyboardShortcutItemList() {
  std::vector<std::unique_ptr<KeyboardShortcutItem>> item_list;
  std::map<int, int> description_shortcuts;
  std::map<int, std::vector<ShortcutCategory>> description_categories;
  std::map<int, std::vector<AcceleratorId>> description_accelerator_ids;
  std::map<int, std::vector<ui::KeyboardCode>> description_replacement_codes;
  for (size_t i = 0; i < kDescriptionToShortcutLength; ++i) {
    const auto& entry = kDescriptionToShortcut[i];
    description_shortcuts[entry.descrption_string_id] =
        entry.shortcut_string_id;
  }

  for (size_t i = 0; i < kDescriptionToCategoryLength; ++i) {
    const auto& entry = kDescriptionToCategory[i];
    description_categories[entry.descrption_string_id].emplace_back(
        entry.category);
  }

  for (size_t i = 0; i < kDescriptionToAcceleratorIdLength; ++i) {
    const auto& entry = kDescriptionToAcceleratorId[i];
    description_accelerator_ids[entry.descrption_string_id].emplace_back(
        entry.accelerator_id);
  }

  // Generate shortcut metadata for each entry in |description_shortcuts|.
  for (const auto& desc_shortcut : description_shortcuts) {
    std::unique_ptr<KeyboardShortcutItem> item =
        std::make_unique<KeyboardShortcutItem>();
    int description_string_id = desc_shortcut.first;
    item->description_string_id = description_string_id;
    item->shortcut_string_id = desc_shortcut.second;
    item->categories = description_categories[description_string_id];
    item->accelerator_ids = description_accelerator_ids[description_string_id];
    // TODO(wutao): |description_replacement_codes| will be provided in the
    // rules 3 & 4 below.
    item->replacement_codes =
        description_replacement_codes[description_string_id];

    // If the |replacement_codes| is empty, we need to dynamically generate the
    // replacement codes.
    //  There are four rules:
    //  1. For regular accelerators, we show all modifiers and key in order,
    //  then
    //     we can auto-generate it by |accelerator_ids| (actually only one
    //     accelerator_id in this case).
    //  2. For grouped |accelerator_ids| with the same modifiers, we can
    //     auto-generate the modifiers. We will not show the key.
    //     E.g. shortcuts for "CTRL + 1 through 8.", we will provide
    //     shortcut_string " + 1 through 8", and auto-generate
    //     {ui::VKEY_CONTROL}.
    //  3. For grouped |accelerator_ids| with different modifiers, e.g.
    //     SHOW_KEYBOARD_OVERLAY, we can not auto-generate it and we will
    //     provide the |replacement_codes|.
    //  4. For ksv items not in the two accelerator_tables, we will provide the
    //     |replacement_codes| and |accelerator_ids| will be empty.
    if (item->replacement_codes.empty()) {
      DCHECK(!item->accelerator_ids.empty());
      // Only use the first accelerator id because the modifiers are the same
      // even if it is a grouped actions. If the modifiers are not the same, we
      // do not know how to generate the replacements, in which we would like to
      // provide replacements infomation directly.
      const AcceleratorId& accelerator_id = item->accelerator_ids[0];
      // Insert shortcut replacements by the order of CTLR, ALT, SHIFT,
      // SEARCH, and then key, to be consistent with how we describe it in the
      // |shortcut_string_id| associated string.
      for (auto modifier : {ui::EF_CONTROL_DOWN, ui::EF_ALT_DOWN,
                            ui::EF_SHIFT_DOWN, ui::EF_COMMAND_DOWN}) {
        if (accelerator_id.modifiers & modifier)
          item->replacement_codes.emplace_back(GetModifierKeyCode(modifier));
      }
      // For non grouped actions, we need to replace the key as well.
      if (item->accelerator_ids.size() == 1)
        item->replacement_codes.emplace_back(accelerator_id.keycode);
    }
    item_list.emplace_back(std::move(item));
  }
  return item_list;
}

}  // namespace ksv
