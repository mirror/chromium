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
    case ax::mojom::Role::kDesktop:
    case ax::mojom::Role::kNone:
    case ax::mojom::Role::kRootWebArea:
    case ax::mojom::Role::kSvgRoot:
    case ax::mojom::Role::kUnknown:
    case ax::mojom::Role::kWebArea:
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

#if defined(AX_CHECKS)
static std::string GetViewAncestryDebugString(views::View* view) {
  std::string view_ancestry;  // e.g. BrowserView>OmniboxView.
  std::string kSeparator = " > ";
  while (view) {
    view_ancestry = kSeparator + view->GetClassName() + view_ancestry;
    view = view->parent();
  }
  return view_ancestry.substr(kSeparator.length());  // Remove first separator.
}

static bool HasGoodText(ui::AXNodeData* data) {
  if (data->GetNameFrom() == ax::mojom::NameFrom::kAttributeExplicitlyEmpty)
    return true;  // No name is intentional.
  if (!data->GetStringAttribute(ax::mojom::StringAttribute::kName).empty())
    return true;  // Has a non-empty name
  if (data->role == ax::mojom::Role::kTextField &&
      !data->GetStringAttribute(ax::mojom::StringAttribute::kPlaceholder)
           .empty())
    return true;  // Textfields are allowed to rely on a placeholder instead.

  return false;
}

static void CheckAccessibility(views::View* view, ui::AXNodeData* data) {
  // TODO(aleventhal) For name check, we might want just focus_behavior()
  if (!view->IsAccessibilityFocusable())
    return;  // Currently no checks for unfocusable items.

  // Focusable views have are visible + enabled, have a name/label.
  const bool is_visible = view->IsDrawn();
  const bool is_enabled = view->enabled();
  const bool has_good_text = HasGoodText(data);

  if (is_visible && is_enabled && has_good_text)
    return;  // Passed all checks.

  // Log the accessibility error.
  LOG(ERROR) << "Accessibility error in " << GetViewAncestryDebugString(view)
             << " (id " << view->id() << "):\n";
  if (!is_visible)
    LOG(ERROR) << "Focusable view not visible";
  if (!is_enabled)
    LOG(ERROR) << "Focusable view not enabled";
  if (!has_good_text)
    LOG(ERROR) << "Focusable view does not have a name, placeholder or "
                  "nameFrom = ax::mojom::NameFrom::kAttributeExplicitlyEmpty";
  LOG(ERROR) << "Constructor stack for debugging:\n"
             << view->GetConstructorStackForDebugging()->ToString()
             << "\n-------------------------------------";
  DCHECK(false);
}
#endif  // AX_CHECKS

void ViewAccessibility::GetAccessibleNodeData(ui::AXNodeData* data) const {
  // Views may misbehave if their widget is closed; return an unknown role
  // rather than possibly crashing.
  if (!owner_view_->GetWidget() || owner_view_->GetWidget()->IsClosed()) {
    data->role = ax::mojom::Role::kUnknown;
    data->SetRestriction(ax::mojom::Restriction::kDisabled);
    return;
  }

  owner_view_->GetAccessibleNodeData(data);
  if (custom_data_.role != ax::mojom::Role::kUnknown)
    data->role = custom_data_.role;

  // TODO(aleventhal) Automatically compute name from:
  // data_.GetIntListAttribute(ax::mojom::IntListAttribute::kLabelledbyIds, ...)
  if (custom_data_.HasStringAttribute(ax::mojom::StringAttribute::kName)) {
    data->SetName(
        custom_data_.GetStringAttribute(ax::mojom::StringAttribute::kName));
  }

  data->location = gfx::RectF(owner_view_->GetBoundsInScreen());
  if (!data->HasStringAttribute(ax::mojom::StringAttribute::kDescription)) {
    base::string16 description;
    owner_view_->GetTooltipText(gfx::Point(), &description);
    data->AddStringAttribute(ax::mojom::StringAttribute::kDescription,
                             base::UTF16ToUTF8(description));
  }

  data->AddStringAttribute(ax::mojom::StringAttribute::kClassName,
                           owner_view_->GetClassName());

  if (owner_view_->IsAccessibilityFocusable())
    data->AddState(ax::mojom::State::kFocusable);

  if (!owner_view_->enabled())
    data->SetRestriction(ax::mojom::Restriction::kDisabled);

  if (!owner_view_->IsDrawn())
    data->AddState(ax::mojom::State::kInvisible);

  if (owner_view_->context_menu_controller())
    data->AddAction(ax::mojom::Action::kShowContextMenu);

#if defined(AX_CHECKS)
  CheckAccessibility(owner_view_, data);
#endif  // AX_CHECKS
}

void ViewAccessibility::OverrideRole(ax::mojom::Role role) {
  DCHECK(IsValidRoleForViews(role));

  custom_data_.role = role;
}

void ViewAccessibility::OverrideName(const std::string& name) {
  custom_data_.SetName(name);
}

void ViewAccessibility::OverrideName(const base::string16& name) {
  custom_data_.SetName(base::UTF16ToUTF8(name));
}

void ViewAccessibility::OverrideDescription(const std::string& description) {
  DCHECK(!custom_data_.HasStringAttribute(
      ax::mojom::StringAttribute::kDescription));
  custom_data_.AddStringAttribute(ax::mojom::StringAttribute::kDescription,
                                  description);
}

gfx::NativeViewAccessible ViewAccessibility::GetNativeObject() {
  return nullptr;
}

}  // namespace views
