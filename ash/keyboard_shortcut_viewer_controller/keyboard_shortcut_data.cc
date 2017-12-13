// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/keyboard_shortcut_viewer_controller/keyboard_shortcut_data.h"

#include "base/macros.h"
#include "components/strings/grit/components_strings.h"

// TODO(wutao): These are some examples how we can organize the metadata. Full
// mappings will be implemented.

namespace ash {

// TODO(wutao): This array will be used to check every action shuold have one
// corresponding keyboard shortcut info metadata.
const ActionToGroupedAction kActionToGroupedAction[] = {
    {SCALE_UI_UP, CHANGE_SCREEN_RESOLUTION},
    {SCALE_UI_DOWN, CHANGE_SCREEN_RESOLUTION}};
const size_t kActionToGroupedActionLength = arraysize(kActionToGroupedAction);

const ActionToKeyboardShortcutItemInfo kActionToKeyboardShortcutItemInfo[] = {
    {LOCK_SCREEN,
     // KeyboardShortcutItemInfo
     {
         // |categories|
         {IDS_KEYBOARD_SHORTCUT_VIEWER_CATEGORY_POPULAR,
          IDS_KEYBOARD_SHORTCUT_VIEWER_CATEGORY_SYSTEM},
         // |descriptoin|
         IDS_KEYBOARD_SHORTCUT_VIEWER_DESCRIPTION_LOCK_SCREEN,
         // |shortcut|
         IDS_KEYBOARD_SHORTCUT_VIEWER_SHORTCUT_LOCK_SCREEN,
         // |shortcut_replacements_info| will be dynamically generated in
         // KeyboardShortcutViewerController.
     }}};
const size_t kActionToKeyboardShortcutItemInfoLength =
    arraysize(kActionToKeyboardShortcutItemInfo);

const GroupedActionToKeyboardShortcutItemInfo
    kGroupedActionToKeyboardShortcutItemInfo[] = {
        {CHANGE_SCREEN_RESOLUTION,
         // KeyboardShortcutItemInfo
         {// |categories|
          {IDS_KEYBOARD_SHORTCUT_VIEWER_CATEGORY_POPULAR,
           IDS_KEYBOARD_SHORTCUT_VIEWER_CATEGORY_SYSTEM},
          // |descriptoin|
          IDS_KEYBOARD_SHORTCUT_VIEWER_DESCRIPTION_CHANGE_SCREEN_RESOLUTION,
          // |shortcut|
          IDS_KEYBOARD_SHORTCUT_VIEWER_SHORTCUT_CHANGE_SCREEN_RESOLUTION,
          // |shortcut_replacements_info|
          {// keyboard_shortcut_viewer::ShortcutReplacementInfo: {id, type}
           {ui::EF_CONTROL_DOWN, keyboard_shortcut_viewer::MODIFIER},
           {ui::EF_SHIFT_DOWN, keyboard_shortcut_viewer::MODIFIER},
           {ui::VKEY_OEM_PLUS, keyboard_shortcut_viewer::VKEY},
           {ui::VKEY_OEM_MINUS, keyboard_shortcut_viewer::VKEY}}}}};
const size_t kGroupedActionToKeyboardShortcutItemInfoLength =
    arraysize(kGroupedActionToKeyboardShortcutItemInfo);

}  // namespace ash
