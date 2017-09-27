// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_COCOA_SCOPED_MENU_BAR_LOCK_H_
#define UI_BASE_COCOA_SCOPED_MENU_BAR_LOCK_H_

#include "base/macros.h"
#include "ui/base/ui_base_export.h"

namespace ui {

// ScopedMenuBarLock uses an AppKit API to lock the menu bar in its current
// state (visible/hidden). Useful to temporarily keep the menu bar visible
// after appearing when the user moved the mouse to the top of the screen, or
// to temporarily prevent the menu bar from showing automatically.
// ScopedMenuBarLock helps enforce that lock/unlock calls are balanced.
class UI_BASE_EXPORT ScopedMenuBarLock {
 public:
  ScopedMenuBarLock();
  ~ScopedMenuBarLock();

 private:
  DISALLOW_COPY_AND_ASSIGN(ScopedMenuBarLock);
};

}  // namespace ui

#endif  // UI_BASE_COCOA_SCOPED_MENU_BAR_LOCK_H_
