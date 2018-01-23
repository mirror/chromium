// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_VIEWER_MODEL_OBSERVER_H_
#define UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_VIEWER_MODEL_OBSERVER_H_

namespace keyboard_shortcut_viewer {

class KeyboardShortcutViewerModelObserver {
 public:
  virtual void OnKeyboardShortcutViewerModelChanged() = 0;

 protected:
  virtual ~KeyboardShortcutViewerModelObserver() {}
};

}  // namespace keyboard_shortcut_viewer

#endif  // UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_VIEWER_MODEL_OBSERVER_H_
