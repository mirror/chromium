// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_TABLET_MODE_TABLET_MODE_SCREENSHOT_CONTROLLER_H_
#define ASH_WM_TABLET_MODE_TABLET_MODE_SCREENSHOT_CONTROLLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "base/macros.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "chromeos/dbus/power_manager_client.h"
#include "ui/events/event_handler.h"

namespace ash {

class ASH_EXPORT TabletModeScreenshotController
    : public ui::EventHandler,
      public chromeos::PowerManagerClient::Observer {
 public:
  TabletModeScreenshotController();
  ~TabletModeScreenshotController() override;

  bool ScreenshotHandled();

  // Overridden from ui::EventHandler:
  void OnKeyEvent(ui::KeyEvent* event) override;

  // Overridden from chromeos::PowerManagerClient::Observer:
  void PowerButtonEventReceived(bool down,
                                const base::TimeTicks& timestamp) override;

 private:
  std::unique_ptr<base::TickClock> tick_clock_;

  base::TimeTicks last_volume_down_time_;
  base::TimeTicks last_power_button_down_time_;

  DISALLOW_COPY_AND_ASSIGN(TabletModeScreenshotController);
};

}  // namespace ash

#endif  // ASH_WM_TABLET_MODE_TABLET_MODE_SCREENSHOT_CONTROLLER_H_
