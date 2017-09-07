// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_PRESS_AND_HOLD_ESC_TRACKER_H_
#define CHROME_BROWSER_UI_VIEWS_PRESS_AND_HOLD_ESC_TRACKER_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/timer/timer.h"
#include "chrome/browser/ui/views/exit_fullscreen_indicator.h"

namespace views {

class Widget;

}  // namespace views

class PressAndHoldEscTracker {
 public:
  explicit PressAndHoldEscTracker(views::Widget* parent_widget);
  ~PressAndHoldEscTracker();

  void StartTracking(const base::Closure& done_callback);
  void CancelTracking();
  bool is_tracking() const { return hold_timer_.IsRunning(); }

 private:
  void OnTrackingDone(const base::Closure& callback);

  ExitFullscreenIndicator exit_indicator_;
  base::OneShotTimer hold_timer_;
  base::OneShotTimer animation_delay_timer_;

  DISALLOW_COPY_AND_ASSIGN(PressAndHoldEscTracker);
};

#endif  // CHROME_BROWSER_UI_VIEWS_PRESS_AND_HOLD_ESC_TRACKER_H_
