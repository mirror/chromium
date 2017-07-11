// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/scheduler/device_status_listener.h"

#include "base/power_monitor/power_monitor.h"

namespace download {

namespace {

// The default delay to notify the observer when network changes from
// disconnected to connected.
const base::TimeDelta kNotifyNetworkOnlineDelay =
    base::TimeDelta::FromSeconds(5);

// Converts |on_battery_power| to battery status.
BatteryStatus ToBatteryStatus(bool on_battery_power) {
  return on_battery_power ? BatteryStatus::NOT_CHARGING
                          : BatteryStatus::CHARGING;
}

// Converts a ConnectionType to NetworkStatus.
NetworkStatus ToNetworkStatus(net::NetworkChangeNotifier::ConnectionType type) {
  switch (type) {
    case net::NetworkChangeNotifier::CONNECTION_ETHERNET:
    case net::NetworkChangeNotifier::CONNECTION_WIFI:
      return NetworkStatus::UNMETERED;
    case net::NetworkChangeNotifier::CONNECTION_2G:
    case net::NetworkChangeNotifier::CONNECTION_3G:
    case net::NetworkChangeNotifier::CONNECTION_4G:
      return NetworkStatus::METERED;
    case net::NetworkChangeNotifier::CONNECTION_UNKNOWN:
    case net::NetworkChangeNotifier::CONNECTION_NONE:
    case net::NetworkChangeNotifier::CONNECTION_BLUETOOTH:
      return NetworkStatus::DISCONNECTED;
  }
  NOTREACHED();
  return NetworkStatus::DISCONNECTED;
}

}  // namespace

DeviceStatusListener::DeviceStatusListener()
    : DeviceStatusListener(kNotifyNetworkOnlineDelay) {}

DeviceStatusListener::DeviceStatusListener(const base::TimeDelta& delay)
    : observer_(nullptr), listening_(false), delay_(delay) {}

DeviceStatusListener::~DeviceStatusListener() {
  Stop();
}

const DeviceStatus& DeviceStatusListener::CurrentDeviceStatus() const {
  DCHECK(listening_) << "Call Start() before querying the status.";
  return status_;
}

void DeviceStatusListener::Start(DeviceStatusListener::Observer* observer) {
  if (listening_)
    return;

  DCHECK(observer);
  observer_ = observer;
  base::PowerMonitor* power_monitor = base::PowerMonitor::Get();
  DCHECK(power_monitor);
  power_monitor->AddObserver(this);

  net::NetworkChangeNotifier::AddConnectionTypeObserver(this);

  status_.battery_status =
      ToBatteryStatus(base::PowerMonitor::Get()->IsOnBatteryPower());
  status_.network_status =
      ToNetworkStatus(net::NetworkChangeNotifier::GetConnectionType());
  listening_ = true;
}

void DeviceStatusListener::Stop() {
  if (!listening_)
    return;

  base::PowerMonitor::Get()->RemoveObserver(this);
  net::NetworkChangeNotifier::RemoveConnectionTypeObserver(this);

  status_ = DeviceStatus();
  listening_ = false;
  observer_ = nullptr;
}

void DeviceStatusListener::OnConnectionTypeChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  NetworkStatus new_status = ToNetworkStatus(type);
  if (status_.network_status == new_status)
    return;

  bool change_to_online =
      (status_.network_status == NetworkStatus::DISCONNECTED) &&
      (new_status != NetworkStatus::DISCONNECTED);
  status_.network_status = new_status;

  if (change_to_online) {
    // It's unreliable to send requests immediately after the network changes
    // from disconnected to connected, notify the observer after a delay.
    timer_.Start(FROM_HERE, delay_, this,
                 &DeviceStatusListener::NotifyStatusChange);
  } else {
    timer_.Stop();
    NotifyStatusChange();
  }
}

void DeviceStatusListener::OnPowerStateChange(bool on_battery_power) {
  status_.battery_status = ToBatteryStatus(on_battery_power);
  NotifyStatusChange();
}

void DeviceStatusListener::NotifyStatusChange() {
  observer_->OnDeviceStatusChanged(status_);
}

}  // namespace download
