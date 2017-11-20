// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/user_activity_logger.h"

#include "base/timer/timer.h"
#include "chrome/browser/chromeos/power/ml/user_activity_event.pb.h"
#include "ui/base/user_activity/user_activity_observer.h"

namespace chromeos {
namespace power {
namespace ml {

UserActivityLogger::UserActivityLogger(LoggingHelper* const helper)
    : idle_event_observed_(false), logging_helper_(helper) {}

UserActivityLogger::~UserActivityLogger() {}

void UserActivityLogger::OnIdleEventObserved(
    const IdleEventNotifier::ActivityData& activity_data) {
  idle_event_observed_ = true;
  activity_data_ = activity_data;
}

// Only log when we observe an idle event.
void UserActivityLogger::OnUserActivity(const ui::Event* /* unused */) {
  if (idle_event_observed_) {
    // Build UserActivityEvent::Event.
    UserActivityEvent::Event event;
    event.set_type(UserActivityEvent::Event::REACTIVATE);
    event.set_reason(UserActivityEvent::Event::USER_ACTIVITY);

    // Build UserActivityEvent::Features.
    UserActivityEvent::Features features;
    features.set_last_activity_time_sec(
        (base::TimeTicks::Now() - activity_data_.last_activity_time)
            .InSeconds());

    // Build UserActivityEvent.
    UserActivityEvent activity_event;
    *activity_event.mutable_event() = event;
    *activity_event.mutable_features() = features;

    // Log to metrics.
    logging_helper_->LogActivity(activity_event);
    idle_event_observed_ = false;
  }
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
