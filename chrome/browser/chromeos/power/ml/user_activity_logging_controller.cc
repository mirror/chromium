// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/user_activity_logging_controller.h"

#include "chrome/browser/chromeos/power/ml/user_activity_logger_ukm.h"
#include "services/viz/public/interfaces/compositing/video_detector_observer.mojom.h"

namespace chromeos {
namespace power {
namespace ml {

UserActivityLoggingController::UserActivityLoggingController() {
  viz::mojom::VideoDetectorObserverPtr observer;
  idle_event_notifier_ =
      std::make_unique<IdleEventNotifier>(mojo::MakeRequest(&observer));
  user_activity_logger_delegate_ = std::make_unique<UserActivityLoggerUKM>();
  user_activity_logger_ = std::make_unique<UserActivityLogger>(
      user_activity_logger_delegate_.get());
}

UserActivityLoggingController::~UserActivityLoggingController() {
  user_activity_logger_.reset();
  user_activity_logger_delegate_.reset();
  idle_event_notifier_.reset();
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
