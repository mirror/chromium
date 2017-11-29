// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/user_activity_logging_controller.h"

#include "chrome/browser/chromeos/power/ml/user_activity_logger_delegate_ukm.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "components/session_manager/session_manager_types.h"
#include "services/viz/public/interfaces/compositing/video_detector_observer.mojom.h"

namespace chromeos {
namespace power {
namespace ml {

UserActivityLoggingController::UserActivityLoggingController()
    : power_manager_client_(
          chromeos::DBusThreadManager::Get()->GetPowerManagerClient()),
      detector_(ui::UserActivityDetector::Get()),
      session_manager_(session_manager::SessionManager::Get()) {
  DCHECK(power_manager_client_);
  DCHECK(detector_);
  DCHECK(session_manager_);

  // TODO(jiameng): both IdleEventNotifier and UserActivityLogger implement
  // viz::mojom::VideoDetectorObserver. We should refactor the code to create
  // one shared video detector observer class.
  viz::mojom::VideoDetectorObserverPtr video_observer_idle_notifier;
  idle_event_notifier_ = std::make_unique<IdleEventNotifier>(
      power_manager_client_, detector_,
      mojo::MakeRequest(&video_observer_idle_notifier));

  viz::mojom::VideoDetectorObserverPtr video_observer_user_logger;
  user_activity_logger_ = std::make_unique<UserActivityLogger>(
      &user_activity_logger_delegate_, idle_event_notifier_.get(), detector_,
      power_manager_client_, session_manager_,
      mojo::MakeRequest(&video_observer_user_logger));
}

UserActivityLoggingController::~UserActivityLoggingController() = default;

}  // namespace ml
}  // namespace power
}  // namespace chromeos
