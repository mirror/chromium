// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_KEYBOARD_SHORTCUT_VIEWER_KEYBOARD_SHORTCUT_VIEWER_CONTROLLER_H_
#define ASH_KEYBOARD_SHORTCUT_VIEWER_KEYBOARD_SHORTCUT_VIEWER_CONTROLLER_H_

#include <vector>

#include "ash/ash_export.h"
#include "ash/public/interfaces/keyboard_shortcut_viewer_controller.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace keyboard_shortcut_viewer {
struct KeyboardShortcutItemInfo;
}

namespace ash {

// The class to initialize the widget to hold KeyboardShortcutView and gather
// shortcuts info at both ash and chrome sides.
class ASH_EXPORT KeyboardShortcutViewerController
    : public mojom::KeyboardShortcutViewerController {
 public:
  using KeyboardShortcutItemInfoList =
      std::vector<keyboard_shortcut_viewer::KeyboardShortcutItemInfo>;

  KeyboardShortcutViewerController();
  ~KeyboardShortcutViewerController() override;

  // Binds the mojo interface to this object.
  void BindRequest(mojom::KeyboardShortcutViewerControllerRequest request);

  // mojom::KeyboardShortcutViewerController:
  void SetClient(
      mojom::KeyboardShortcutViewerControllerClientPtr client) override;
  void UpdateKeyboardShortcutsInfo(
      const KeyboardShortcutItemInfoList& shortcuts_info) override;

  void GetClientKeyboardShortcutsInfo();
  void OpenKeyboardShortcutViewer();

 private:
  void ShowKeyboardShortcutViewer(
      const KeyboardShortcutItemInfoList& shortcuts_info);

  // Bindings for users of the mojo interface.
  mojo::BindingSet<mojom::KeyboardShortcutViewerController> bindings_;

  // Client interface in chrome back to keyboard shortcut viewer.
  mojom::KeyboardShortcutViewerControllerClientPtr client_;

  // Ash side keyboard shortcuts info.
  KeyboardShortcutItemInfoList ash_shortcuts_info_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardShortcutViewerController);
};

}  // namespace ash

#endif  // ASH_KEYBOARD_SHORTCUT_VIEWER_KEYBOARD_SHORTCUT_VIEWER_CONTROLLER_H_
