// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "build/buildflag.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/ui_features.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/view.h"

namespace {

// Return helpful string for identifying a view, e.g.
// BrowserView > OmniboxView (id 3) [view construction stack].
static std::string GetViewDebugString(views::View* view) {
  // Start with view id.
  std::string view_debug_info = base::StringPrintf(" (id %d)", view->id());

  // Prepend ncestry info.
  std::string kSeparator = " > ";
  while (view) {
    view_debug_info = kSeparator + view->GetClassName() + view_debug_info;
    view = view->parent();
  }
  // Remove first separator.
  view_debug_info = view_debug_info.substr(kSeparator.length());

  return view_debug_info;
}

static std::string ViewConstructionStack(views::View* view) {
  std::string view_construction_info =
      "\n\nThe following stack created the inaccessible View:\n";
  // Add the stack that created the view
  view_construction_info += view->GetConstructorStackForDebugging()->ToString();
  return view_construction_info;
}

static bool CheckBadText(ui::AXNodeData& data) {
  // Focusable nodes must have an accessible name, otherwise screen reader users
  // will not know what they landed on. For example, the reload button should
  // have an accessible name of "Reload".
  // Exceptions:
  // 1) Textfields can set the placeholder string attribute.
  // 2) Explictly setting the name to "" is allowed if the view uses.
  // AXNodedata.SetIntAttribute(ax::mojom::IntAttribute::NameFrom,
  //                    ax::mojom::NameFrom::kAttributeExplicitlyEmpty)

  return data.GetNameFrom() != ax::mojom::NameFrom::kAttributeExplicitlyEmpty &&
         data.GetStringAttribute(ax::mojom::StringAttribute::kName).empty() &&
         data.GetStringAttribute(ax::mojom::StringAttribute::kPlaceholder)
             .empty();
}

static bool CheckDisabled(ui::AXNodeData& data) {
  return data.GetRestriction() == ax::mojom::Restriction::kDisabled;
}

static bool CheckInvisible(ui::AXNodeData& data) {
  return data.HasState(ax::mojom::State::kInvisible);
}

static bool CheckViewAccessibility(views::View* view,
                                   std::string* error_message) {
  views::ViewAccessibility& view_ax = view->GetViewAccessibility();
  ui::AXNodeData node_data;
  view_ax.GetAccessibleNodeData(&node_data);

  std::string violations;

  // No checks for unfocusable items yet.
  if (view->IsAccessibilityFocusable()) {
    if (CheckBadText(node_data))
      violations +=
          "\n- Focusable View has no accessible name or placeholder, and the "
          "name attribute does not use kAttributeExplicitlyEmpty.";
    if (CheckDisabled(node_data))
      violations += "\n- Focusable View should not be disabled.";
    if (CheckInvisible(node_data))
      violations += "\n- Focusable View should not be invisible.";
  }

  if (violations.empty())
    return true;  // No errors.

  *error_message = std::string("View violates RunUIAccessibilityChecks():\n") +
                   GetViewDebugString(view) + violations +
                   ViewConstructionStack(view);
  return false;
}

static bool CheckViewSubtreeAccessibility(views::View* view,
                                          std::string* error_message) {
  if (!CheckViewAccessibility(view, error_message))
    return false;
  for (int i = 0; i < view->child_count(); ++i) {
    if (!CheckViewSubtreeAccessibility(view->child_at(i), error_message))
      return false;
  }

  return true;  // All views in this subtree passed all checks.
}

}  // namespace

bool RunUIAccessibilityChecks(std::string* error_message) {
#if defined(OS_MACOSX) && !BUILDFLAG(MAC_VIEWS_BROWSER)
  return true;
#else
  const BrowserList* active_browser_list = BrowserList::GetInstance();
  for (size_t index = 0; index < active_browser_list->size(); index++) {
    Browser* browser = active_browser_list->get(index);
    if (browser->IsAttemptingToCloseBrowser())
      continue;
    BrowserWindow* browser_window = browser->window();
    if (!browser_window) {
      *error_message = base::StringPrintf(
          "Accessibility error: no browser window for browser %zu", index);
      return false;
    }
    BrowserView* browser_view = static_cast<BrowserView*>(browser_window);

    return CheckViewSubtreeAccessibility(browser_view, error_message);
  }

  return true;
#endif  // OS_MACOSX && !BUILDFLAG(MAC_VIEWS_BROWSER)
}
