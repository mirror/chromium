// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_IME_MODIFIER_KEY_H_
#define UI_BASE_IME_MODIFIER_KEY_H_

namespace ui {

enum ModifierKey {
  kSearchKey = 0,  // Customizable.
  kControlKey,     // Customizable.
  kAltKey,         // Customizable.
  kVoidKey,
  kCapsLockKey,   // Customizable.
  kEscapeKey,     // Customizable.
  kBackspaceKey,  // Customizable.
  // IMPORTANT: Add new key to the end, because the keys may have been stored
  // in user preferences.
  kNumModifierKeys,
};

}  // namespace ui

#endif  // UI_BASE_IME_MODIFIER_KEY_H_
