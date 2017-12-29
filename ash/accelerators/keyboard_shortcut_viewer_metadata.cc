// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/accelerators/keyboard_shortcut_viewer_metadata.h"

#include "ash/strings/grit/ash_strings.h"
#include "base/macros.h"
#include "components/strings/grit/components_strings.h"

// TODO(wutao): These are some examples how we can organize the metadata. Full
// mappings will be implemented.

namespace ash {

KSVItemInfoToActions::KSVItemInfoToActions(
    keyboard_shortcut_viewer::KSVItemInfo shortcut_info,
    std::vector<AcceleratorAction> actions)
    : shortcut_info(shortcut_info), actions(actions) {}
KSVItemInfoToActions::~KSVItemInfoToActions() = default;

const KSVItemInfoToActions kKSVItemInfoToActions[] = {
    { // KeyboardShortcutItemInfo
     {// |category_ids|
      {IDS_SHORTCUT_VIEWER_CATEGORY_POPULAR,
       IDS_SHORTCUT_VIEWER_CATEGORY_SYSTEM},
      // |description_id|
      IDS_ASH_SHORTCUT_DESCRIPTION_LOCK_SCREEN,
      // |shortcut_id|
      IDS_ASH_SHORTCUT_SHORTCUT_ONE_MODIFIER_ONE_KEY,
      // |replacement_ids| and |replacement_types| will be dynamically generated
      // in MakeAshKeyboardShortcutViewerMetadata().
      {},
      {}},
     // |actions|
     {ash::LOCK_SCREEN}},

    // Some accelerators are grouped into one metadata for the
    // KeyboardShortcutViewer due to the similarity. E.g. the shortcut for
    // "Change screen resolution" is "Ctrl + Shift and + or -", which groups two
    // accelerators.
    // TODO(wutao): Full list will be implemented.
    { // KeyboardShortcutItemInfo
     {// |category_ids|
      {IDS_SHORTCUT_VIEWER_CATEGORY_SYSTEM},
      // |description_id|
      IDS_ASH_SHORTCUT_DESCRIPTION_CHANGE_SCREEN_RESOLUTION,
      // |shortcut_id|
      IDS_ASH_SHORTCUT_SHORTCUT_CHANGE_SCREEN_RESOLUTION,
      // |replacement_ids| and |replacement_types| will be dynamically generated
      // in MakeAshKeyboardShortcutViewerMetadata().
      {},
      {}},
     // |actions|
     {ash::SCALE_UI_UP, ash::SCALE_UI_DOWN}}};
const size_t kKSVItemInfoToActionsLength = arraysize(kKSVItemInfoToActions);

}  // namespace ash
