// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/accessibility/view_accessibility.h"
#include "ui/accessibility/ax_node_data.h"

namespace views {

ViewAccessibility::ViewAccessibility(View* view) : view_(view) {}

ui::AXNodeData ViewAccessibility::GetAccessibleNodeData() {
  ui::AXNodeData data;

  // Views may misbehave if their widget is closed; return an unknown role
  // rather than possibly crashing.
  if (!view_->GetWidget() || view_->GetWidget()->IsClosed()) {
    data.role = ui::AX_ROLE_UNKNOWN;
    data.AddIntAttribute(ui::AX_ATTR_RESTRICTION, ui::AX_RESTRICTION_DISABLED);
    return data;
  }

  view_->GetAccessibleNodeData(&data);
  if (custom_data_.role != ui::AX_ROLE_UNKNOWN)
    data.role = custom_data_.role;
  if (!custom_data_.GetStringAttribute(ui::AX_ATTR_NAME).empty())
    data.SetName(custom_data_.GetStringAttribute(ui::AX_ATTR_NAME));

  data.location = GetBoundsInScreen();
  base::string16 description;
  view_->GetTooltipText(gfx::Point(), &description);
  data.AddStringAttribute(ui::AX_ATTR_DESCRIPTION,
                          base::UTF16ToUTF8(description));

  if (view_->IsAccessibilityFocusable())
    data.AddState(ui::AX_STATE_FOCUSABLE);

  if (!view_->enabled()) {
    data.AddIntAttribute(ui::AX_ATTR_RESTRICTION, ui::AX_RESTRICTION_DISABLED);
  }

  if (!view_->IsDrawn())
    data.AddState(ui::AX_STATE_INVISIBLE);

  if (view_->context_menu_controller())
    data.AddAction(ui::AX_ACTION_SHOW_CONTEXT_MENU);

  // Make sure this element is excluded from the a11y tree if there's a
  // focusable parent. All keyboard focusable elements should be leaf nodes.
  // Exceptions to this rule will themselves be accessibility focusable.
  if (IsViewUnfocusableChildOfFocusableAncestor(view_))
    data.role = ui::AX_ROLE_IGNORED;

  return data;
}

void ViewAccessibility::SetRole(ui::AXRole role) {
  custom_data_.role = role;
}

void ViewAccessibility::SetName(std::string name) {
  custom_data_.SetName(name);
}

gfx::NativeViewAccessible ViewAccessibility::GetNativeObject() {
  return nullptr;
}

}  // namespace views
