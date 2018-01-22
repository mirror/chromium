// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/accessibility/ax_role_properties.h"

namespace ui {

bool IsRoleClickable(ax::mojom::Role role) {
  switch (role) {
    case ax::mojom::Role::BUTTON:
    case ax::mojom::Role::CHECK_BOX:
    case ax::mojom::Role::COLOR_WELL:
    case ax::mojom::Role::DISCLOSURE_TRIANGLE:
    case ax::mojom::Role::LINK:
    case ax::mojom::Role::LIST_BOX_OPTION:
    case ax::mojom::Role::MENU_BUTTON:
    case ax::mojom::Role::MENU_ITEM:
    case ax::mojom::Role::MENU_ITEM_CHECK_BOX:
    case ax::mojom::Role::MENU_ITEM_RADIO:
    case ax::mojom::Role::MENU_LIST_OPTION:
    case ax::mojom::Role::MENU_LIST_POPUP:
    case ax::mojom::Role::POP_UP_BUTTON:
    case ax::mojom::Role::RADIO_BUTTON:
    case ax::mojom::Role::SWITCH:
    case ax::mojom::Role::TAB:
    case ax::mojom::Role::TOGGLE_BUTTON:
      return true;
    default:
      return false;
  }
}

bool IsDocument(ax::mojom::Role role) {
  switch (role) {
    case ax::mojom::Role::ROOT_WEB_AREA:
    case ax::mojom::Role::WEB_AREA:
      return true;
    default:
      return false;
  }
}

bool IsCellOrTableHeaderRole(ax::mojom::Role role) {
  switch (role) {
    case ax::mojom::Role::CELL:
    case ax::mojom::Role::COLUMN_HEADER:
    case ax::mojom::Role::ROW_HEADER:
      return true;
    default:
      return false;
  }
}

bool IsTableLikeRole(ax::mojom::Role role) {
  switch (role) {
    case ax::mojom::Role::TABLE:
    case ax::mojom::Role::GRID:
    case ax::mojom::Role::TREE_GRID:
      return true;
    default:
      return false;
  }
}

bool IsContainerWithSelectableChildrenRole(ax::mojom::Role role) {
  switch (role) {
    case ax::mojom::Role::COMBO_BOX_GROUPING:
    case ax::mojom::Role::COMBO_BOX_MENU_BUTTON:
    case ax::mojom::Role::GRID:
    case ax::mojom::Role::LIST_BOX:
    case ax::mojom::Role::MENU:
    case ax::mojom::Role::MENU_BAR:
    case ax::mojom::Role::RADIO_GROUP:
    case ax::mojom::Role::TAB_LIST:
    case ax::mojom::Role::TOOLBAR:
    case ax::mojom::Role::TREE:
    case ax::mojom::Role::TREE_GRID:
      return true;
    default:
      return false;
  }
}

bool IsRowContainer(ax::mojom::Role role) {
  switch (role) {
    case ax::mojom::Role::TREE:
    case ax::mojom::Role::TREE_GRID:
    case ax::mojom::Role::GRID:
    case ax::mojom::Role::TABLE:
      return true;
    default:
      return false;
  }
}

bool IsControl(ax::mojom::Role role) {
  switch (role) {
    case ax::mojom::Role::BUTTON:
    case ax::mojom::Role::CHECK_BOX:
    case ax::mojom::Role::COLOR_WELL:
    case ax::mojom::Role::COMBO_BOX_MENU_BUTTON:
    case ax::mojom::Role::DISCLOSURE_TRIANGLE:
    case ax::mojom::Role::LIST_BOX:
    case ax::mojom::Role::MENU:
    case ax::mojom::Role::MENU_BAR:
    case ax::mojom::Role::MENU_BUTTON:
    case ax::mojom::Role::MENU_ITEM:
    case ax::mojom::Role::MENU_ITEM_CHECK_BOX:
    case ax::mojom::Role::MENU_ITEM_RADIO:
    case ax::mojom::Role::MENU_LIST_OPTION:
    case ax::mojom::Role::MENU_LIST_POPUP:
    case ax::mojom::Role::POP_UP_BUTTON:
    case ax::mojom::Role::RADIO_BUTTON:
    case ax::mojom::Role::SCROLL_BAR:
    case ax::mojom::Role::SEARCH_BOX:
    case ax::mojom::Role::SLIDER:
    case ax::mojom::Role::SPIN_BUTTON:
    case ax::mojom::Role::SWITCH:
    case ax::mojom::Role::TAB:
    case ax::mojom::Role::TEXT_FIELD:
    case ax::mojom::Role::TEXT_FIELD_WITH_COMBO_BOX:
    case ax::mojom::Role::TOGGLE_BUTTON:
    case ax::mojom::Role::TREE:
      return true;
    default:
      return false;
  }
}

bool IsMenuRelated(ax::mojom::Role role) {
  switch (role) {
    case ax::mojom::Role::MENU:
    case ax::mojom::Role::MENU_BAR:
    case ax::mojom::Role::MENU_BUTTON:
    case ax::mojom::Role::MENU_ITEM:
    case ax::mojom::Role::MENU_ITEM_CHECK_BOX:
    case ax::mojom::Role::MENU_ITEM_RADIO:
    case ax::mojom::Role::MENU_LIST_OPTION:
    case ax::mojom::Role::MENU_LIST_POPUP:
      return true;
    default:
      return false;
  }
}

}  // namespace ui
