// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/menu_item_builder.h"

#include <stddef.h>

#include "content/public/common/menu_item.h"

namespace content {

MenuItem MenuItemBuilder::Build(const blink::WebMenuItemInfo& item) {
  MenuItem result;

  result.label = item.label.Utf16();
  result.tool_tip = item.tool_tip.Utf16();
  result.type = static_cast<MenuItem::Type>(item.type);
  result.action = item.action;
  result.rtl = (item.text_direction == blink::kWebTextDirectionRightToLeft);
  result.has_directional_override = item.has_text_direction_override;
  result.enabled = item.enabled;
  result.checked = item.checked;
  for (const auto& sub_menu_item : item.sub_menu_items)
    result.submenu.push_back(MenuItemBuilder::Build(sub_menu_item));

  return result;
}

}  // namespace content
