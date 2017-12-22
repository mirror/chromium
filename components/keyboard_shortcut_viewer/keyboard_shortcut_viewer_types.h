// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_SHORTCUT_VIEWER_KEYBOARD_SHORTCUT_VIEWER_TYPES_H_
#define COMPONENTS_KEYBOARD_SHORTCUT_VIEWER_KEYBOARD_SHORTCUT_VIEWER_TYPES_H_

#include "components/keyboard_shortcut_viewer/common/keyboard_shortcut_info.mojom.h"

namespace keyboard_shortcut_viewer {

using KSVController = mojom::KeyboardShortcutViewerController;
using KSVControllerPtr = mojom::KeyboardShortcutViewerControllerPtr;

using KSVControllerRequest = mojom::KeyboardShortcutViewerControllerRequest;

using KSVClient = mojom::KeyboardShortcutViewerControllerClient;
using KSVClientPtr = mojom::KeyboardShortcutViewerControllerClientPtr;

using KSVItemInfo = mojom::KeyboardShortcutItemInfo;
using KSVItemInfoPtrList = std::vector<mojom::KeyboardShortcutItemInfoPtr>;

using KSVReplacementType = mojom::ShortcutReplacementType;

}  // namespace keyboard_shortcut_viewer

#endif  // COMPONENTS_KEYBOARD_SHORTCUT_VIEWER_KEYBOARD_SHORTCUT_VIEWER_TYPES_H_
