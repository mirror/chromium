// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_MENU_SOURCE_TYPE_H_
#define UI_BASE_MENU_SOURCE_TYPE_H_

namespace ui {

// TODO(varunjain): Remove MENU_SOURCE_NONE (crbug.com/250964)
// A Java counterpart will be generated for this enum.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.ui.base
enum MenuSourceType {
  MENU_SOURCE_NONE = 0,
  MENU_SOURCE_MOUSE = 1,
  MENU_SOURCE_KEYBOARD = 2,
  MENU_SOURCE_TOUCH = 3,
  MENU_SOURCE_TOUCH_EDIT_MENU = 4,
  MENU_SOURCE_LONG_PRESS = 5,
  MENU_SOURCE_LONG_TAP = 6,
  MENU_SOURCE_TOUCH_HANDLE = 7,
  MENU_SOURCE_STYLUS = 8,
  MENU_SOURCE_ADJUST_SELECTION = 9,
  MENU_SOURCE_ADJUST_SELECTION_RESET = 10,
  MENU_SOURCE_TYPE_LAST = MENU_SOURCE_ADJUST_SELECTION_RESET
};

}  // namespace ui

#endif  // UI_BASE_MENU_SOURCE_TYPE_H_
