// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/press_and_hold_esc_tracker.h"

#include "base/bind.h"

namespace {

// Time in milliseconds to hold the Esc key in order to exit full screen.
const int kHoldEscapeTimeMs = 1000;

const int kAnimationDelayMs = 250;

}  // namespace

PressAndHoldEscTracker::PressAndHoldEscTracker(views::Widget* parent_widget)
    : exit_indicator_(parent_widget) {}

PressAndHoldEscTracker::~PressAndHoldEscTracker() {}

void PressAndHoldEscTracker::StartTracking(const base::Closure& done_callback) {
  if (is_tracking()) {
    return;
  }

  hold_timer_.Start(FROM_HERE,
                    base::TimeDelta::FromMilliseconds(kHoldEscapeTimeMs),
                    base::Bind(&PressAndHoldEscTracker::OnTrackingDone,
                               base::Unretained(this), done_callback));

  animation_delay_timer_.Start(
      FROM_HERE, base::TimeDelta::FromMilliseconds(kAnimationDelayMs),
      base::Bind(&ExitFullscreenIndicator::Show,
                 base::Unretained(&exit_indicator_)));
}

void PressAndHoldEscTracker::CancelTracking() {
  if (!is_tracking()) {
    return;
  }
  animation_delay_timer_.Stop();
  hold_timer_.Stop();
  exit_indicator_.Hide(true);
}

void PressAndHoldEscTracker::OnTrackingDone(const base::Closure& callback) {
  exit_indicator_.Hide(false);
  callback.Run();
}
