// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/display/display_configuration_observer.h"

#include "ash/display/window_tree_host_manager.h"
#include "ash/shell.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/command_line.h"
#include "chrome/browser/chromeos/display/display_preferences.h"
#include "chromeos/chromeos_switches.h"
#include "ui/display/manager/display_manager.h"

namespace chromeos {

DisplayConfigurationObserver::DisplayConfigurationObserver() {
  ash::Shell::Get()->window_tree_host_manager()->AddObserver(this);
}

DisplayConfigurationObserver::~DisplayConfigurationObserver() {
  ash::Shell::Get()->tablet_mode_controller()->RemoveObserver(this);
  ash::Shell::Get()->window_tree_host_manager()->RemoveObserver(this);
}

void DisplayConfigurationObserver::OnDisplaysInitialized() {
  ash::Shell::Get()->tablet_mode_controller()->AddObserver(this);
  // Update the display pref with the initial power state.
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(chromeos::switches::kFirstExecAfterBoot) &&
      !suppressed_)
    StoreDisplayPrefs();
}

void DisplayConfigurationObserver::OnDisplayConfigurationChanged() {
  if (!suppressed_)
    StoreDisplayPrefs();
}

void DisplayConfigurationObserver::OnTabletModeStarted() {
  suppressed_ = true;
  LOG(ERROR) << "OnTabletModeStarted";
  // TODO(oshima): Mirroring won't work with 3 displays.
  display::DisplayManager* display_manager = ash::Shell::Get()->display_manager();
  was_in_mirror_mode_ = display_manager->IsInMirrorMode();
  display_manager->SetMirrorMode(true);
}

void DisplayConfigurationObserver::OnTabletModeEnded() {
  LOG(ERROR) << "OnTabletModeEnded";
  if (!was_in_mirror_mode_)
    ash::Shell::Get()->display_manager()->SetMirrorMode(false);
  suppressed_ = false;
}

}  // namespace chromeos
