// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_VIEWER_METADATA_H_
#define UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_VIEWER_METADATA_H_

#include <vector>

#include "ui/chromeos/ksv/keyboard_shortcut_viewer_export.h"
#include "ui/chromeos/ksv/models/keyboard_shortcut_item.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace ksv {

// We had to duplicate the key and modifiers to map to an accelerator while not
// have deps on //ash and //chrome. Currently this mapping is only used by
// //chrome/browser/ui/ash/ksv/keyboard_shortcut_viewer_metadata_unittest.
struct KSV_EXPORT AcceleratorId {
  bool operator<(const AcceleratorId& other) const;

  ui::KeyboardCode keycode;
  int modifiers;
};

// Gathers the needed metadata of accelerators for KeyboardShortcutViewer.
struct KSV_EXPORT KSVItemToAcceleratorIds {
  KSVItemToAcceleratorIds(KeyboardShortcutItem ksv_item,
                          std::vector<AcceleratorId> accelerator_ids);
  explicit KSVItemToAcceleratorIds(const KSVItemToAcceleratorIds& other);
  ~KSVItemToAcceleratorIds();

  KeyboardShortcutItem ksv_item;

  // Multiple accelerators can be mapped to the same metadata.
  std::vector<AcceleratorId> accelerator_ids;
};

// Returns a list of mapping from KSVItem to |accelerator_ids|.
KSV_EXPORT std::vector<KSVItemToAcceleratorIds>
GetKSVItemToAcceleratorIdsList();

}  // namespace ksv

#endif  // UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_VIEWER_METADATA_H_
