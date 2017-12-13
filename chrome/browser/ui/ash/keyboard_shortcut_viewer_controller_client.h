// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_KEYBOARD_SHORTCUT_VIEWER_CONTROLLER_CLIENT_H_
#define CHROME_BROWSER_UI_ASH_KEYBOARD_SHORTCUT_VIEWER_CONTROLLER_CLIENT_H_

#include <vector>

#include "ash/public/interfaces/keyboard_shortcut_viewer_controller.mojom.h"
#include "components/keyboard_shortcut_viewer/keyboard_shortcut_info.h"
#include "mojo/public/cpp/bindings/binding.h"

// Connects the controller in ash.
class KeyboardShortcutViewerControllerClient
    : public ash::mojom::KeyboardShortcutViewerControllerClient {
 public:
  using KeyboardShortcutItemInfoList =
      std::vector<keyboard_shortcut_viewer::KeyboardShortcutItemInfo>;

  KeyboardShortcutViewerControllerClient();
  ~KeyboardShortcutViewerControllerClient() override;

  // Initializes and connects to ash.
  void Init();

  static KeyboardShortcutViewerControllerClient* Get();

  // ash::mojom::KeyboardShortcutViewerControllerClient:
  void GetKeyboardShortcutsInfo() override;

  // Wrappers around the mojom::KeyboardShortcutViewerController interface.
  void UpdateKeyboardShortcutsInfo(
      const KeyboardShortcutItemInfoList& shortcuts_info);

 private:
  // Binds this object to its mojo interface and sets it as the ash client.
  void BindAndSetClient();

  // Binds this object to the mojo interface.
  mojo::Binding<ash::mojom::KeyboardShortcutViewerControllerClient> binding_;

  // KeyboardShortcutViewerController interface in ash.
  ash::mojom::KeyboardShortcutViewerControllerPtr
      keyboard_shortcut_viewer_controller_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardShortcutViewerControllerClient);
};

#endif  // CHROME_BROWSER_UI_ASH_KEYBOARD_SHORTCUT_VIEWER_CONTROLLER_CLIENT_H_
