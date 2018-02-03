// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/overview/overview_utils.h"

#include "ash/public/cpp/ash_switches.h"
#include "ash/shell.h"
#include "ash/wm/splitview/split_view_controller.h"
#include "ash/wm/window_state.h"
#include "base/command_line.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/window.h"

namespace ash {

bool CanCoverFullscreen(aura::Window* window) {
  SplitViewController* split_view_controller =
      Shell::Get()->split_view_controller();
  if (split_view_controller->IsSplitViewModeActive())
    return split_view_controller->CanSnap(window);

  ui::WindowShowState show_state =
      window->GetProperty(aura::client::kShowStateKey);
  return show_state == ui::SHOW_STATE_FULLSCREEN ||
         show_state == ui::SHOW_STATE_MAXIMIZED;
}

bool IsNewOverviewAnimations() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      ash::switches::kAshEnableNewOverviewAnimations);
}

bool IsNewOverviewUi() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      ash::switches::kAshEnableNewOverviewUi);
}

}  // namespace ash
