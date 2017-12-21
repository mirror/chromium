// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_ACCELERATORS_KEYBOARD_SHORTCUT_VIEWER_METADATA_H_
#define ASH_ACCELERATORS_KEYBOARD_SHORTCUT_VIEWER_METADATA_H_

#include <vector>

#include "ash/accelerators/accelerator_table.h"
#include "ash/ash_export.h"
#include "components/keyboard_shortcut_viewer/common/keyboard_shortcut_info.mojom.h"

namespace ash {

using KSVItemInfoPtrList =
    std::vector<keyboard_shortcut_viewer::mojom::KeyboardShortcutItemInfoPtr>;

// Return the metadata of ash side accelerators for the KeyboardShortcutViewer.
ASH_EXPORT KSVItemInfoPtrList MakeAshKeyboardShortcutViewerMetadata();

}  // namespace ash

#endif  // ASH_ACCELERATORS_KEYBOARD_SHORTCUT_VIEWER_METADATA_H_
