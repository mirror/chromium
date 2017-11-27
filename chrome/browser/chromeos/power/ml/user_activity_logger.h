// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_LOGGER_H_
#define CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_LOGGER_H_

#include "base/macros.h"
#include "base/optional.h"
#include "base/sequenced_task_runner.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/chromeos/power/ml/idle_event_notifier.h"
#include "chrome/browser/chromeos/power/ml/user_activity_event.pb.h"
#include "chrome/browser/chromeos/power/ml/user_activity_logger_delegate.h"
#include "chromeos/dbus/power_manager/idle.pb.h"
#include "chromeos/dbus/power_manager_client.h"
#include "services/viz/public/interfaces/compositing/video_detector_observer.mojom.h"
#include "ui/base/user_activity/user_activity_observer.h"

namespace chromeos {
namespace power {
namespace ml {

// Logs user activity after an idle event is observed.
// TODO(renjieliu): Add power-related activity as well.
class UserActivityLogger : public ui::UserActivityObserver,
                           public IdleEventNotifier::Observer,
                           public PowerManagerClient::Observer,
                           public viz::mojom::VideoDetectorObserver {
 public:
  explicit UserActivityLogger(
      UserActivityLoggerDelegate* delegate,
      IdleEventNotifier* idle_event_notifier,
      chromeos::PowerManagerClient* power_manager_client,
      viz::mojom::VideoDetectorObserverRequest request);
  ~UserActivityLogger() override;

  // ui::UserActivityObserver overrides.
  void OnUserActivity(const ui::Event* event) override;

  // chromeos::PowerManagerClient::Observer overrides:
  void LidEventReceived(chromeos::PowerManagerClient::LidState state,
                        const base::TimeTicks& timestamp) override;
  void PowerChanged(const power_manager::PowerSupplyProperties& proto) override;
  void TabletModeEventReceived(chromeos::PowerManagerClient::TabletMode mode,
                               const base::TimeTicks& timestamp) override;
  void ScreenIdleStateChanged(
      const power_manager::ScreenIdleState& proto) override;

  // viz::mojom::VideoDetectorObserver overrides:
  void OnVideoActivityStarted() override;
  void OnVideoActivityEnded() override {}  // We don't care about this.

 private:
  friend class UserActivityLoggerTest;

  // IdleEventNotifier::Observer overrides.
  void OnIdleEventObserved(
      const IdleEventNotifier::ActivityData& data) override;

  // Update lid state and tablet mode from received switch states.
  void OnReceiveSwitchStates(
      base::Optional<chromeos::PowerManagerClient::SwitchStates> switch_states);

  // Extract features from last known activity data and from device states.
  void ExtractFeatures(const IdleEventNotifier::ActivityData& activity_data);

  // Log event only when an idle event is observed.
  void MaybeLogEvent(UserActivityEvent::Event::Type type,
                     UserActivityEvent::Event::Reason reason);

  // Set the task runner for testing purpose.
  void SetTaskRunnerForTesting(
      scoped_refptr<base::SequencedTaskRunner> task_runner);

  // Flag indicating whether an idle event has been observed.
  bool idle_event_observed_ = false;

  chromeos::PowerManagerClient::LidState lid_state_ =
      chromeos::PowerManagerClient::LidState::NOT_PRESENT;

  chromeos::PowerManagerClient::TabletMode tablet_mode_ =
      chromeos::PowerManagerClient::TabletMode::UNSUPPORTED;

  // Device type.
  UserActivityEvent::Features::DeviceType device_type_;

  // Flag indicating whether the device is on battery.
  bool on_battery_ = false;

  // Battery percent.
  base::Optional<float> battery_percent_;

  // Features extracted when receives an idle event.
  UserActivityEvent::Features features_;

  // Logger delegate.
  UserActivityLoggerDelegate* const logger_delegate_;

  // Idle event notifier.
  IdleEventNotifier* const idle_event_notifier_;

  // Power manager client.
  chromeos::PowerManagerClient* const power_manager_client_;

  // Video detector observer binding.
  mojo::Binding<viz::mojom::VideoDetectorObserver> binding_;

  base::TimeDelta idle_delay_ = base::TimeDelta::FromSeconds(10);

  // Timer to be triggered when a screen idle event is triggered.
  base::OneShotTimer screen_idle_timer_;

  DISALLOW_COPY_AND_ASSIGN(UserActivityLogger);
};

}  // namespace ml
}  // namespace power
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_LOGGER_H_
