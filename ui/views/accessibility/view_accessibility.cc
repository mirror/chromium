// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/accessibility/view_accessibility.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/base/ui_features.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace views {

namespace {

bool IsValidRoleForViews(ax::mojom::Role role) {
  switch (role) {
    // These roles all have special meaning and shouldn't ever be
    // set on a View.
    case ax::mojom::Role::DESKTOP:
    case ax::mojom::Role::NONE:
    case ax::mojom::Role::ROOT_WEB_AREA:
    case ax::mojom::Role::SVG_ROOT:
    case ax::mojom::Role::UNKNOWN:
    case ax::mojom::Role::WEB_AREA:
      return false;

    default:
      return true;
  }
}

}  // namespace

#if !BUILDFLAG_INTERNAL_HAS_NATIVE_ACCESSIBILITY()
// static
std::unique_ptr<ViewAccessibility> ViewAccessibility::Create(View* view) {
  return base::WrapUnique(new ViewAccessibility(view));
}
#endif

ViewAccessibility::ViewAccessibility(View* view) : owner_view_(view) {}

ViewAccessibility::~ViewAccessibility() {}

const ui::AXUniqueId& ViewAccessibility::GetUniqueId() const {
  return unique_id_;
}

void ViewAccessibility::GetAccessibleNodeData(ui::AXNodeData* data) const {
  // Views may misbehave if their widget is closed; return an unknown role
  // rather than possibly crashing.
  if (!owner_view_->GetWidget() || owner_view_->GetWidget()->IsClosed()) {
    data->role = ax::mojom::Role::UNKNOWN;
    data->AddIntAttribute(ax::mojom::IntAttribute::RESTRICTION,
                          static_cast<int32_t>(ax::mojom::Restriction::DISABLED));
    return;
  }

  owner_view_->GetAccessibleNodeData(data);
  if (custom_data_.role != ax::mojom::Role::UNKNOWN)
    data->role = custom_data_.role;
  if (custom_data_.HasStringAttribute(ax::mojom::StringAttribute::NAME))
    data->SetName(custom_data_.GetStringAttribute(ax::mojom::StringAttribute::NAME));

  data->location = gfx::RectF(owner_view_->GetBoundsInScreen());
  if (!data->HasStringAttribute(ax::mojom::StringAttribute::DESCRIPTION)) {
    base::string16 description;
    owner_view_->GetTooltipText(gfx::Point(), &description);
    data->AddStringAttribute(ax::mojom::StringAttribute::DESCRIPTION,
                             base::UTF16ToUTF8(description));
  }

  data->AddStringAttribute(ax::mojom::StringAttribute::CLASS_NAME, owner_view_->GetClassName());

  if (owner_view_->IsAccessibilityFocusable())
    data->AddState(ax::mojom::State::FOCUSABLE);

  if (!owner_view_->enabled())
    data->AddIntAttribute(ax::mojom::IntAttribute::RESTRICTION,
                          static_cast<int32_t>(ax::mojom::Restriction::DISABLED));

  if (!owner_view_->visible())
    data->AddState(ax::mojom::State::INVISIBLE);

  if (owner_view_->context_menu_controller())
    data->AddAction(ax::mojom::Action::SHOW_CONTEXT_MENU);
}

void ViewAccessibility::OverrideRole(ax::mojom::Role role) {
  DCHECK(IsValidRoleForViews(role));

  custom_data_.role = role;
}

void ViewAccessibility::OverrideName(const std::string& name) {
  custom_data_.SetName(name);
}

gfx::NativeViewAccessible ViewAccessibility::GetNativeObject() {
  return nullptr;
}

}  // namespace views
