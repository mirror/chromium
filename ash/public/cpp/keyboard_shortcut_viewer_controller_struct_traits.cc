// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/public/cpp/keyboard_shortcut_viewer_controller_struct_traits.h"

namespace mojo {

bool StructTraits<ash::mojom::ShortcutReplacementInfoDataView,
                  keyboard_shortcut_viewer::ShortcutReplacementInfo>::
    Read(ash::mojom::ShortcutReplacementInfoDataView data,
         keyboard_shortcut_viewer::ShortcutReplacementInfo* out) {
  out->replacement_id = data.replacement_id();
  if (!data.ReadReplacementType(&out->replacement_type))
    return false;
  return true;
}

bool StructTraits<ash::mojom::KeyboardShortcutItemInfoDataView,
                  keyboard_shortcut_viewer::KeyboardShortcutItemInfo>::
    Read(ash::mojom::KeyboardShortcutItemInfoDataView data,
         keyboard_shortcut_viewer::KeyboardShortcutItemInfo* out) {
  std::vector<int> categories;
  if (!data.ReadCategories(&categories))
    return false;
  out->categories = std::move(categories);

  out->description = data.description();
  out->shortcut = data.shortcut();

  std::vector<keyboard_shortcut_viewer::ShortcutReplacementInfo>
      shortcut_replacements_info;
  if (!data.ReadShortcutReplacementsInfo(&shortcut_replacements_info))
    return false;
  out->shortcut_replacements_info = std::move(shortcut_replacements_info);

  return true;
}

}  // namespace mojo
