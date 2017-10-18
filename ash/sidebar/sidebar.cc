// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/sidebar/sidebar.h"

#include "ash/sidebar/sidebar_widget.h"
#include "ui/aura/window.h"

#include "ash/public/cpp/shell_window_ids.h"
#include "ash/root_window_controller.h"
#include "ash/shell.h"

namespace ash {

Sidebar::Sidebar() {}
Sidebar::~Sidebar() {}

// static
Sidebar* Sidebar::ForWindow(const aura::Window* window) {
  DCHECK(window);
  DCHECK(Shell::HasInstance());
  auto* root_window_controller = RootWindowController::ForWindow(window);
  return root_window_controller ? root_window_controller->sidebar() : nullptr;
}

void Sidebar::SetRootAndShelf(aura::Window* root, Shelf* shelf) {
  DCHECK(!widget_);
  DCHECK(!root_);
  DCHECK(!shelf_);

  root_ = root;
  shelf_ = shelf;
}

void Sidebar::Show(SidebarMode mode) {
  DCHECK(!widget_);
  DCHECK(root_);
  DCHECK(shelf_);
  aura::Window* container =
      root_->GetChildById(kShellWindowId_ShelfBubbleContainer);
  widget_.reset(new SidebarWidget(container, this, shelf_, mode));
  widget_->Show();
}

void Sidebar::Hide() {
  DCHECK(widget_);
  widget_->Hide();
  widget_.reset();
}

bool Sidebar::IsVisible() const {
  return widget_ && widget_->IsVisible();
}

}  // namespace ash
