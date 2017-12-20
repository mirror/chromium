// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCELERATORS_KEYBOARD_SHORTCUT_VIEWER_CONTROLLER_H_
#define ASH_ACCELERATORS_KEYBOARD_SHORTCUT_VIEWER_CONTROLLER_H_

#include <vector>

#include "ash/ash_export.h"
#include "components/keyboard_shortcut_viewer/common/keyboard_shortcut_info.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace ash {

using KSVController =
    keyboard_shortcut_viewer::mojom::KeyboardShortcutViewerController;

// The class to gather the shortcuts metadata for the KeyboardShortcutViewer.
class ASH_EXPORT KeyboardShortcutViewerController : public KSVController {
 public:
  using KSVClientPtr = keyboard_shortcut_viewer::mojom::
      KeyboardShortcutViewerControllerClientPtr;
  using KSVControllerRequest =
      keyboard_shortcut_viewer::mojom::KeyboardShortcutViewerControllerRequest;
  using KSVItemInfoPtrList =
      std::vector<keyboard_shortcut_viewer::mojom::KeyboardShortcutItemInfoPtr>;

  KeyboardShortcutViewerController();
  ~KeyboardShortcutViewerController() override;

  // Binds the mojo interface to this object.
  void BindRequest(KSVControllerRequest request);

  // keyboard_shortcut_viewer::mojom::KeyboardShortcutViewerController:
  void SetClient(KSVClientPtr client) override;
  void OpenKeyboardShortcutViewer() override;

 private:
  void ShowKeyboardShortcutViewer(KSVItemInfoPtrList shortcuts_info);

  // Bindings for users of the mojo interface.
  mojo::BindingSet<KSVController> bindings_;

  // Client interface in chrome.
  KSVClientPtr client_;

  base::WeakPtrFactory<KeyboardShortcutViewerController> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardShortcutViewerController);
};

}  // namespace ash

#endif  // ASH_ACCELERATORS_KEYBOARD_SHORTCUT_VIEWER_CONTROLLER_H_
