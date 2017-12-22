// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_KEYBOARD_SHORTCUT_VIEWER_CONTROLLER_CLIENT_H_
#define CHROME_BROWSER_UI_ASH_KEYBOARD_SHORTCUT_VIEWER_CONTROLLER_CLIENT_H_

#include <vector>

#include "components/keyboard_shortcut_viewer/common/keyboard_shortcut_info.mojom.h"
#include "components/keyboard_shortcut_viewer/keyboard_shortcut_viewer_types.h"
#include "mojo/public/cpp/bindings/binding.h"

// Connects the controller in ash.
class KeyboardShortcutViewerControllerClient
    : public keyboard_shortcut_viewer::KSVClient {
 public:
  KeyboardShortcutViewerControllerClient();
  ~KeyboardShortcutViewerControllerClient() override;

  // Initializes and connects to ash.
  void Init();

  static KeyboardShortcutViewerControllerClient* Get();

  // keyboard_shortcut_viewer::mojom::KeyboardShortcutViewerControllerClient:
  void GetKeyboardShortcutsInfo(
      GetKeyboardShortcutsInfoCallback callback) override;

  // Wrapper around the
  // keyboard_shortcut_viewer::mojom::KeyboardShortcutViewerController
  // interface.
  void OpenKeyboardShortcutViewer();

 private:
  // Binds this object to its mojo interface and sets it as the ash client.
  void BindAndSetClient();

  // Binds this object to the mojo interface.
  mojo::Binding<keyboard_shortcut_viewer::KSVClient> binding_;

  // KeyboardShortcutViewerController interface in ash.
  keyboard_shortcut_viewer::KSVControllerPtr ksv_controller_;

  keyboard_shortcut_viewer::KSVItemInfoPtrList MakeChromeShortcutsMetadata();

  DISALLOW_COPY_AND_ASSIGN(KeyboardShortcutViewerControllerClient);
};

#endif  // CHROME_BROWSER_UI_ASH_KEYBOARD_SHORTCUT_VIEWER_CONTROLLER_CLIENT_H_
