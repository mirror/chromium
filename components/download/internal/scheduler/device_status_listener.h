// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_SCHEDULER_DEVICE_STATUS_LISTENER_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_SCHEDULER_DEVICE_STATUS_LISTENER_H_

#include <memory>

#include "base/power_monitor/power_observer.h"
#include "base/timer/timer.h"
#include "components/download/internal/scheduler/device_status.h"
#include "components/download/internal/scheduler/network_status_listener.h"
#include "net/base/network_change_notifier.h"

namespace download {

// Helper class to listen to battery changes.
// TODO(xingliu): Move power monitor observer implementation to this class.
class BatteryStatusListener {
 public:
  BatteryStatusListener();
  virtual ~BatteryStatusListener();

  virtual int GetBatteryPercentage();

 private:
  DISALLOW_COPY_AND_ASSIGN(BatteryStatusListener);
};

// Listens to network and battery status change and notifies the observer.
class DeviceStatusListener : public NetworkStatusListener::Observer,
                             public base::PowerObserver {
 public:
  class Observer {
   public:
    // Called when device status is changed.
    virtual void OnDeviceStatusChanged(const DeviceStatus& device_status) = 0;
  };

  explicit DeviceStatusListener(const base::TimeDelta& startup_delay,
                                const base::TimeDelta& online_delay,
                                const base::TimeDelta& battery_query_interval);
  ~DeviceStatusListener() override;

  // Returns the current device status for download scheduling. May updates
  // internal device status when called.
  const DeviceStatus& CurrentDeviceStatus();

  // Starts/stops to listen network and battery change events, virtual for
  // testing.
  virtual void Start(DeviceStatusListener::Observer* observer);
  virtual void Stop();

 protected:
  // Creates cross platform instances, visible for testing.
  virtual void BuildPlatformListeners();

  // Used to listen to network connectivity changes.
  std::unique_ptr<NetworkStatusListener> network_listener_;

  // Used to listen to battery status.
  std::unique_ptr<BatteryStatusListener> battery_listener_;

  // The current device status.
  DeviceStatus status_;

  // The observer that listens to device status change events.
  Observer* observer_;

  // If device status listener is started.
  bool listening_;

 private:
  // Start after a delay to wait for potential network stack setup.
  void StartAfterDelay();

  // NetworkStatusListener::Observer implementation.
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  // base::PowerObserver implementation.
  void OnPowerStateChange(bool on_battery_power) override;

  // Notifies the observer about device status change.
  void NotifyStatusChange();

  // Called after a delay to notify the observer. See |delay_|.
  void NotifyNetworkChange(NetworkStatus network_status);

  // Updates battery percentage. Will throttle based on
  // |battery_query_interval_| when |force| is false.
  void UpdateBatteryPercentage(bool force);

  // Used to start the device listener or notify network change after a delay.
  base::OneShotTimer timer_;

  // The delay used on start up.
  base::TimeDelta startup_delay_;

  // The delay used when network status becomes online.
  base::TimeDelta online_delay_;

  // Interval to throttle battery queries. Cached value will be returned inside
  // this interval.
  base::TimeDelta battery_query_interval_;

  // Time stamp to record last battery query.
  base::Time last_battery_query_;

  DISALLOW_COPY_AND_ASSIGN(DeviceStatusListener);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_SCHEDULER_DEVICE_STATUS_LISTENER_H_
