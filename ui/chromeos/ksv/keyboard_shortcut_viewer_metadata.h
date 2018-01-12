// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_VIEWER_METADATA_H_
#define UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_VIEWER_METADATA_H_

#include <vector>

#include "ui/chromeos/ksv/keyboard_shortcut_item.h"
#include "ui/chromeos/ksv/ksv_export.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace ksv {

// Returns a list of KSVItem.
KSV_EXPORT const std::vector<KeyboardShortcutItem>& GetKSVItemList();

}  // namespace ksv

#endif  // UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_VIEWER_METADATA_H_
