// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_DISPLAY_PERSISTENT_WINDOW_CONTROLLER_H_
#define ASH_DISPLAY_PERSISTENT_WINDOW_CONTROLLER_H_

#include "ash/ash_export.h"
#include "ash/display/window_tree_host_manager.h"
#include "base/macros.h"
#include "ui/display/display_observer.h"
#include "ui/display/manager/chromeos/display_configurator.h"
#include "ui/display/screen.h"

namespace display {
class Display;
class Screen;
}  // namespace display

namespace ash {

class ASH_EXPORT PersistentWindowController
    : public display::DisplayConfigurator::Observer,
      public display::DisplayObserver,
      public WindowTreeHostManager::Observer {
 public:
  explicit PersistentWindowController(
      WindowTreeHostManager* window_tree_host_manager);
  ~PersistentWindowController() override;

  void SavePersistentWindowBounds(
      const std::vector<display::Display>& displays);

 private:
  void OnDisplayModeChanged(
      const display::DisplayConfigurator::DisplayStateList& displays) override;

  // display::DisplayObserver:
  void OnDisplayRemoved(const display::Display& old_display) override;

  // WindowTreeHostManager::Observer:
  void OnDisplayConfigurationChanged() override;

  display::Screen* screen_;  // Not owned.
  WindowTreeHostManager* window_tree_host_manager_;  // Not owned.

  bool configured_ = false;

  DISALLOW_COPY_AND_ASSIGN(PersistentWindowController);
};

}  // namespace ash

#endif  // ASH_DISPLAY_PERSISTENT_WINDOW_CONTROLLER_H_
