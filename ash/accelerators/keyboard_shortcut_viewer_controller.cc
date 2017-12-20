// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accelerators/keyboard_shortcut_viewer_controller.h"

#include "ash/accelerators/accelerator_table.h"
#include "ash/accelerators/keyboard_shortcut_viewer_metadata.h"
#include "base/bind.h"

namespace ash {

KeyboardShortcutViewerController::KeyboardShortcutViewerController()
    : weak_ptr_factory_(this) {}

KeyboardShortcutViewerController::~KeyboardShortcutViewerController() = default;

void KeyboardShortcutViewerController::BindRequest(
    KSVControllerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void KeyboardShortcutViewerController::SetClient(KSVClientPtr client) {
  client_ = std::move(client);
}

// keyboard_shortcut_viewer::mojom::KeyboardShortcutViewerController:
void KeyboardShortcutViewerController::OpenKeyboardShortcutViewer() {
  if (client_) {
    client_->GetKeyboardShortcutsInfo(base::Bind(
        &KeyboardShortcutViewerController::ShowKeyboardShortcutViewer,
        weak_ptr_factory_.GetWeakPtr()));
  } else {
    ShowKeyboardShortcutViewer(KSVItemInfoPtrList());
  }
}

void KeyboardShortcutViewerController::ShowKeyboardShortcutViewer(
    KSVItemInfoPtrList client_shortcuts_info) {
  KSVItemInfoPtrList shortcuts_info = MakeAshKeyboardShortcutViewerMetadata();
  if (client_shortcuts_info.size() > 0) {
    shortcuts_info.insert(
        shortcuts_info.end(),
        std::make_move_iterator(client_shortcuts_info.begin()),
        std::make_move_iterator(client_shortcuts_info.end()));
  }

  // TODO(wutao): To set the |shortcuts_info| to the model of
  // KeyboardShortcutView.
  NOTIMPLEMENTED();
}

}  // namespace ash
