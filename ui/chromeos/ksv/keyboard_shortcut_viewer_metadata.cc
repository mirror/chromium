// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/chromeos/ksv/keyboard_shortcut_viewer_metadata.h"

#include "base/macros.h"
#include "ui/chromeos/strings/grit/ui_chromeos_strings.h"
#include "ui/events/event_constants.h"

namespace {

// TODO(wutao): These are two examples how we organize the metadata. Full
// mappings will be implemented.
const ksv::KSVItemToAcceleratorIds kKSVItemToAcceleratorIds[] = {
    { // KeyboardShortcutItem
     {// |categories|
      {ksv::kPopular, ksv::kSystemAndDisplay},
      // |description_id|
      IDS_KSV_DESCRIPTION_LOCK_SCREEN,
      // |shortcut_id|
      IDS_KSV_SHORTCUT_ONE_MODIFIER_ONE_KEY,
      // |replacement_codes| will be dynamically generated according to
      // accelerator_ids.
      {}},
     // |accelerator_ids|
     {{ui::VKEY_L, ui::EF_COMMAND_DOWN}}},

    // Some accelerators are grouped into one metadata for the
    // KeyboardShortcutViewer due to the similarity. E.g. the shortcut for
    // "Change screen resolution" is "Ctrl + Shift and + or -", which groups two
    // accelerators.
    // TODO(wutao): Full list will be implemented.
    { // KeyboardShortcutItemInfo
     {// |categories|
      {ksv::kSystemAndDisplay},
      // |description_id|
      IDS_KSV_DESCRIPTION_CHANGE_SCREEN_RESOLUTION,
      // |shortcut_id|
      IDS_KSV_SHORTCUT_CHANGE_SCREEN_RESOLUTION,
      // |replacement_codes| will be dynamically generated according to
      // accelerator_ids.
      {}},
     // |accelerator_ids|
     {{ui::VKEY_OEM_MINUS, ui::EF_CONTROL_DOWN | ui::EF_SHIFT_DOWN},
      {ui::VKEY_OEM_PLUS, ui::EF_CONTROL_DOWN | ui::EF_SHIFT_DOWN}}}};

}  // namespace

namespace ksv {

bool AcceleratorId::operator<(const AcceleratorId& other) const {
  if (keycode != other.keycode)
    return keycode < other.keycode;
  return modifiers < other.modifiers;
}

KSVItemToAcceleratorIds::KSVItemToAcceleratorIds(
    KeyboardShortcutItem ksv_item,
    std::vector<AcceleratorId> accelerator_ids)
    : ksv_item(ksv_item), accelerator_ids(accelerator_ids) {}

KSVItemToAcceleratorIds::KSVItemToAcceleratorIds(
    const KSVItemToAcceleratorIds& other) = default;

KSVItemToAcceleratorIds::~KSVItemToAcceleratorIds() = default;

std::vector<KSVItemToAcceleratorIds> GetKSVItemToAcceleratorIdsList() {
  CR_DEFINE_STATIC_LOCAL(std::vector<KSVItemToAcceleratorIds>, item_to_ids,
                         (std::begin(kKSVItemToAcceleratorIds),
                          std::end(kKSVItemToAcceleratorIds)));
  return item_to_ids;
}

}  // namespace ksv
