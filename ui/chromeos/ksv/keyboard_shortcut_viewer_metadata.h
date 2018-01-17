// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_VIEWER_METADATA_H_
#define UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_VIEWER_METADATA_H_

#include <vector>

#include "ui/chromeos/ksv/ksv_export.h"

namespace ksv {

struct KeyboardShortcutItem;

// Returns a list of ash and chrome keyboard shortcuts metadata.
KSV_EXPORT const std::vector<KeyboardShortcutItem>&
GetKeyboardShortcutItemList();

}  // namespace ksv

#endif  // UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_VIEWER_METADATA_H_
