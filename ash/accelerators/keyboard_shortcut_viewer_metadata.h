// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCELERATORS_KEYBOARD_SHORTCUT_VIEWER_METADATA_H_
#define ASH_ACCELERATORS_KEYBOARD_SHORTCUT_VIEWER_METADATA_H_

#include <vector>

#include "ash/accelerators/accelerator_table.h"
#include "ash/ash_export.h"
#include "components/keyboard_shortcut_viewer/common/keyboard_shortcut_info.mojom.h"
#include "components/keyboard_shortcut_viewer/keyboard_shortcut_viewer_types.h"

namespace ash {

// Gathers the needed metadata of accelerators for KeyboardShortcutViewer.
struct KSVItemInfoToActions {
  KSVItemInfoToActions(keyboard_shortcut_viewer::KSVItemInfo shortcut_info,
                       std::vector<AcceleratorAction> actions);
  ~KSVItemInfoToActions();

  keyboard_shortcut_viewer::KSVItemInfo shortcut_info;

  // Many actions can map to the same shortcut metadata.
  std::vector<AcceleratorAction> actions;
};

// Shortcuts metadata handled by KeyboardShortcutViewerController.
ASH_EXPORT extern const KSVItemInfoToActions kKSVItemInfoToActions[];
ASH_EXPORT extern const size_t kKSVItemInfoToActionsLength;

}  // namespace ash

#endif  // ASH_ACCELERATORS_KEYBOARD_SHORTCUT_VIEWER_METADATA_H_
