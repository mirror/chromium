// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/blocked_content/scoped_visibility_tracker.h"

#include <utility>

ScopedVisibilityTracker::ScopedVisibilityTracker(
    std::unique_ptr<base::TickClock> tick_clock,
    bool is_shown)
    : tick_clock_(std::move(tick_clock)) {
  DCHECK(tick_clock_);
  if (is_shown)
    OnShown();
}

ScopedVisibilityTracker::~ScopedVisibilityTracker() {}

void ScopedVisibilityTracker::OnShown() {
  UpdateForegroundDuration();
  last_time_shown_ = tick_clock_->NowTicks();
  currently_in_foreground_ = true;
}

void ScopedVisibilityTracker::OnHidden() {
  UpdateForegroundDuration();
  currently_in_foreground_ = false;
}

base::TimeDelta ScopedVisibilityTracker::GetForegroundDuration() const {
  base::TimeDelta current_foreground_time =
      currently_in_foreground_ ? tick_clock_->NowTicks() - last_time_shown_
                               : base::TimeDelta();
  return foreground_duration_ + current_foreground_time;
}

void ScopedVisibilityTracker::UpdateForegroundDuration() {
  if (currently_in_foreground_) {
    foreground_duration_ += tick_clock_->NowTicks() - last_time_shown_;
  }
}
