// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCELERATORS_KEYBOARD_SHORTCUT_VIEWER_CONTROLLER_H_
#define ASH_ACCELERATORS_KEYBOARD_SHORTCUT_VIEWER_CONTROLLER_H_

#include <vector>

#include "ash/accelerators/accelerator_table.h"
#include "ash/ash_export.h"
#include "components/keyboard_shortcut_viewer/common/keyboard_shortcut_info.mojom.h"
#include "components/keyboard_shortcut_viewer/keyboard_shortcut_viewer_types.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace ash {

// Gathers the shortcuts metadata for the KeyboardShortcutViewer.
class ASH_EXPORT KeyboardShortcutViewerController
    : public keyboard_shortcut_viewer::KSVController {
 public:
  KeyboardShortcutViewerController();
  ~KeyboardShortcutViewerController() override;

  // Binds the mojo interface to this object.
  void BindRequest(keyboard_shortcut_viewer::KSVControllerRequest request);

  // keyboard_shortcut_viewer::mojom::KeyboardShortcutViewerController:
  void SetClient(keyboard_shortcut_viewer::KSVClientPtr client) override;
  void OpenKeyboardShortcutViewer() override;

 private:
  void ShowKeyboardShortcutViewer(
      keyboard_shortcut_viewer::KSVItemInfoPtrList shortcuts_info);

  keyboard_shortcut_viewer::KSVItemInfoPtrList MakeAshShortcutsMetadata();

  // Bindings for users of the mojo interface.
  mojo::BindingSet<keyboard_shortcut_viewer::KSVController> bindings_;

  keyboard_shortcut_viewer::KSVClientPtr client_;

  std::map<AcceleratorAction, std::vector<AcceleratorData>>
      action_to_accelerator_data_;

  base::WeakPtrFactory<KeyboardShortcutViewerController> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardShortcutViewerController);
};

}  // namespace ash

#endif  // ASH_ACCELERATORS_KEYBOARD_SHORTCUT_VIEWER_CONTROLLER_H_
