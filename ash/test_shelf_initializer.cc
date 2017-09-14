// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/test_shelf_initializer.h"

#include "ash/root_window_controller.h"
#include "ash/shelf/shelf.h"
#include "ash/shell.h"
#include "ash/shell_observer.h"

namespace ash {

ShelfInitializer::ShelfInitializer() { Shell::Get()->AddShellObserver(this); }

ShelfInitializer::~ShelfInitializer() { Shell::Get()->RemoveShellObserver(this); }

void ShelfInitializer::OnShelfCreatedForRootWindow(aura::Window* root_window) {
  Shelf* shelf = RootWindowController::ForWindow(root_window)->shelf();
  // Do not override the custom initialization performed by some unit tests.
  if (shelf->alignment() == SHELF_ALIGNMENT_BOTTOM_LOCKED &&
      shelf->auto_hide_behavior() == SHELF_AUTO_HIDE_ALWAYS_HIDDEN) {
    shelf->SetAlignment(SHELF_ALIGNMENT_BOTTOM);
    shelf->SetAutoHideBehavior(SHELF_AUTO_HIDE_BEHAVIOR_NEVER);
    shelf->UpdateVisibilityState();
  }
}

}  // namespace ash
