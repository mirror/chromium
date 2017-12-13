// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_KEYBOARD_SHORTCUT_DATA_H_
#define ASH_KEYBOARD_SHORTCUT_DATA_H_

#include "ash/accelerators/accelerator_table.h"
#include "ash/ash_export.h"
#include "components/keyboard_shortcut_viewer/keyboard_shortcut_info.h"

namespace ash {

// Some accelerators are grouped into one item in the KeyboardShortcutViewer due
// to the similarity.
// TODO(wutao): This is an example. Full list will be implemented.
enum GroupedAction {
  CHANGE_SCREEN_RESOLUTION,
};

struct ActionToGroupedAction {
  AcceleratorAction action;
  GroupedAction grouped_action;
};

struct ActionToKeyboardShortcutItemInfo {
  AcceleratorAction action;
  keyboard_shortcut_viewer::KeyboardShortcutItemInfo shortcut_info;
};

struct GroupedActionToKeyboardShortcutItemInfo {
  GroupedAction grouped_action;
  keyboard_shortcut_viewer::KeyboardShortcutItemInfo shortcut_info;
};

ASH_EXPORT extern const ActionToGroupedAction kActionToGroupedAction[];
ASH_EXPORT extern const size_t kActionToGroupedActionLength;
ASH_EXPORT extern const ActionToKeyboardShortcutItemInfo
    kActionToKeyboardShortcutItemInfo[];
ASH_EXPORT extern const size_t kActionToKeyboardShortcutItemInfoLength;
ASH_EXPORT extern const GroupedActionToKeyboardShortcutItemInfo
    kGroupedActionToKeyboardShortcutItemInfo[];
ASH_EXPORT extern const size_t kGroupedActionToKeyboardShortcutItemInfoLength;

}  // namespace ash

#endif  // ASH_KEYBOARD_SHORTCUT_DATA_H_
