// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/press_and_hold_esc_tracker.h"

#include "base/bind.h"
#include "base/bind_helpers.h"

namespace {

// Time in milliseconds to hold the Esc key in order to exit full screen.
const int kHoldEscapeTimeMs = 1000;

const int kAnimationDelayMs = 250;

}  // namespace

PressAndHoldEscTracker::PressAndHoldEscTracker(gfx::NativeView parent_view)
    : control_popup_(parent_view, base::Bind(&base::DoNothing)) {}

PressAndHoldEscTracker::~PressAndHoldEscTracker() {}

void PressAndHoldEscTracker::StartTracking(
    const gfx::Rect& parent_bounds_in_screen,
    const base::Closure& done_callback) {
  if (is_tracking()) {
    return;
  }

  hold_timer_.Start(FROM_HERE,
                    base::TimeDelta::FromMilliseconds(kHoldEscapeTimeMs),
                    base::Bind(&PressAndHoldEscTracker::OnTrackingDone,
                               base::Unretained(this), done_callback));

  animation_delay_timer_.Start(
      FROM_HERE, base::TimeDelta::FromMilliseconds(kAnimationDelayMs),
      base::Bind(&FullscreenControlPopup::Show,
                 base::Unretained(&control_popup_), parent_bounds_in_screen));
}

void PressAndHoldEscTracker::CancelTracking() {
  if (!is_tracking()) {
    return;
  }
  animation_delay_timer_.Stop();
  hold_timer_.Stop();
  control_popup_.Hide(true);
}

void PressAndHoldEscTracker::OnTrackingDone(const base::Closure& callback) {
  control_popup_.Hide(false);
  callback.Run();
}
