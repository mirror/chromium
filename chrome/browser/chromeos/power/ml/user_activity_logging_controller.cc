// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/user_activity_logging_controller.h"

#include "chrome/browser/chromeos/power/ml/user_activity_logger_ukm.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "services/viz/public/interfaces/compositing/video_detector_observer.mojom.h"

namespace chromeos {
namespace power {
namespace ml {

UserActivityLoggingController::UserActivityLoggingController() {
  chromeos::PowerManagerClient* power_client =
      chromeos::DBusThreadManager::Get()->GetPowerManagerClient();
  DCHECK(power_client);
  viz::mojom::VideoDetectorObserverPtr observer;
  idle_event_notifier_ = std::make_unique<IdleEventNotifier>(
      power_client, mojo::MakeRequest(&observer));
  user_activity_logger_delegate_ = std::make_unique<UserActivityLoggerUKM>();
  // TODO(jiameng): pass |power_client| and |idle_event_notifier_| to
  // UserActivityLogger ctor.
  user_activity_logger_ = std::make_unique<UserActivityLogger>(
      user_activity_logger_delegate_.get());
}

UserActivityLoggingController::~UserActivityLoggingController() = default;

}  // namespace ml
}  // namespace power
}  // namespace chromeos
