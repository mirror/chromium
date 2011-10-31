// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura_shell/desktop_layout_manager.h"

#include "ui/aura/window.h"
#include "views/widget/widget.h"

namespace aura_shell {
namespace internal {

////////////////////////////////////////////////////////////////////////////////
// DesktopLayoutManager, public:

DesktopLayoutManager::DesktopLayoutManager(aura::Window* owner)
    : owner_(owner),
      background_widget_(NULL),
      launcher_widget_(NULL),
      status_area_widget_(NULL) {
}

DesktopLayoutManager::~DesktopLayoutManager() {
}

////////////////////////////////////////////////////////////////////////////////
// DesktopLayoutManager, aura::LayoutManager implementation:

void DesktopLayoutManager::OnWindowResized() {
  gfx::Rect fullscreen_bounds =
      gfx::Rect(owner_->bounds().width(), owner_->bounds().height());

  aura::Window::Windows::const_iterator i;
  for (i = owner_->children().begin(); i != owner_->children().end(); ++i)
    (*i)->SetBounds(fullscreen_bounds);

  background_widget_->SetBounds(fullscreen_bounds);

  gfx::Rect status_area_bounds = status_area_widget_->GetWindowScreenBounds();
  status_area_widget_->SetBounds(
      gfx::Rect(owner_->bounds().right() - status_area_bounds.width(),
                owner_->bounds().bottom() - status_area_bounds.height(),
                status_area_bounds.width(),
                status_area_bounds.height()));

  // Give the launcher the available width minus the status area.
  gfx::Rect launcher_bounds = launcher_widget_->GetWindowScreenBounds();
  launcher_widget_->SetBounds(
      gfx::Rect(0, owner_->bounds().bottom() - launcher_bounds.height(),
                owner_->bounds().width() - status_area_bounds.width(),
                launcher_bounds.height()));
}

void DesktopLayoutManager::OnWindowAdded(aura::Window* child) {
}

void DesktopLayoutManager::OnWillRemoveWindow(aura::Window* child) {
}

void DesktopLayoutManager::OnChildWindowVisibilityChanged(aura::Window* child,
                                                          bool visibile) {
}

void DesktopLayoutManager::CalculateBoundsForChild(
    aura::Window* child, gfx::Rect* requested_bounds) {
}


}  // namespace internal
}  // namespace aura_shell
