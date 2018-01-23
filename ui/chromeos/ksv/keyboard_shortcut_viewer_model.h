// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_VIEWER_MODEL_H_
#define UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_VIEWER_MODEL_H_

#include <memory>
#include <vector>

#include "base/observer_list.h"

namespace keyboard_shortcut_viewer {

enum class ShortcutCategory;
struct KeyboardShortcutItem;
class KeyboardShortcutViewerModelObserver;

// Holds a list of KeyboardShortcutItem.
class KeyboardShortcutViewerModel {
 public:
  KeyboardShortcutViewerModel();
  ~KeyboardShortcutViewerModel();

  void MakeKeyboardShortcutItems();

  void AddObserver(KeyboardShortcutViewerModelObserver* observer);
  void RemoveObserver(KeyboardShortcutViewerModelObserver* observer);

 private:
  std::vector<std::unique_ptr<KeyboardShortcutItem>> shortcut_items_;

  base::ObserverList<KeyboardShortcutViewerModelObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardShortcutViewerModel);
};

}  // namespace keyboard_shortcut_viewer

#endif  // UI_CHROMEOS_KSV_KEYBOARD_SHORTCUT_VIEWER_MODEL_H_
