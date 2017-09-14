// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/discovery_network_monitor_metric_observer.h"

namespace media_router {
namespace {

constexpr base::TimeDelta kConfirmDisconnectTimeout =
    base::TimeDelta::FromSeconds(1);

DiscoveryNetworkMonitorConnectionType ConnectionTypeFromIdAndType(
    const std::string& network_id,
    net::NetworkChangeNotifier::ConnectionType connection_type) {
  if (network_id == DiscoveryNetworkMonitor::kNetworkIdDisconnected) {
    return DiscoveryNetworkMonitorConnectionType::kDisconnected;
  } else if (network_id == DiscoveryNetworkMonitor::kNetworkIdUnknown) {
    switch (connection_type) {
      case net::NetworkChangeNotifier::CONNECTION_WIFI:
        return DiscoveryNetworkMonitorConnectionType::kUnknownReportedAsWifi;
      case net::NetworkChangeNotifier::CONNECTION_ETHERNET:
        return DiscoveryNetworkMonitorConnectionType::
            kUnknownReportedAsEthernet;
      case net::NetworkChangeNotifier::CONNECTION_UNKNOWN:
        return DiscoveryNetworkMonitorConnectionType::kUnknown;
      default:
        return DiscoveryNetworkMonitorConnectionType::kUnknownReportedAsOther;
    }
  } else {
    switch (connection_type) {
      case net::NetworkChangeNotifier::CONNECTION_WIFI:
        return DiscoveryNetworkMonitorConnectionType::kWifi;
      case net::NetworkChangeNotifier::CONNECTION_ETHERNET:
        return DiscoveryNetworkMonitorConnectionType::kEthernet;
      default:
        return DiscoveryNetworkMonitorConnectionType::kUnknown;
    }
  }
}

}  // namespace

DiscoveryNetworkMonitorMetricObserver::DiscoveryNetworkMonitorMetricObserver(
    std::unique_ptr<base::TickClock> tick_clock,
    std::unique_ptr<DiscoveryNetworkMonitorMetrics> metrics)
    : tick_clock_(std::move(tick_clock)),
      metrics_(std::move(metrics)),
      disconnect_timer_(tick_clock_.get()) {
  DCHECK(tick_clock_);
  DCHECK(metrics_);
}

DiscoveryNetworkMonitorMetricObserver::
    ~DiscoveryNetworkMonitorMetricObserver() {}

void DiscoveryNetworkMonitorMetricObserver::OnNetworksChanged(
    const std::string& network_id) {
  network_id_ = network_id;
  auto now = tick_clock_->NowTicks();
  if (last_event_time_ &&
      network_id == DiscoveryNetworkMonitor::kNetworkIdDisconnected) {
    timer_().Start(FROM_HERE, kConfirmDisconnectTimeout,
                   base::Bind(&DiscoveryNetworkMonitorMetricObserver::
                                  ConfirmDisconnectedToReportMetrics,
                              base::Unretained(this), now));
    return;
  } else if (last_event_time_) {
    metrics_->RecordTimeBetweenNetworkChangeEvents(now - *last_event_time_);
  }
  last_event_time_ = now;
  timer_().Stop();
  DiscoveryNetworkMonitorConnectionType connection_type =
      ConnectionTypeFromIdAndType(
          network_id, net::NetworkChangeNotifier::GetConnectionType());
  metrics_->RecordConnectionType(connection_type);
}

void DiscoveryNetworkMonitorMetricObserver::SetTimerForTest(
    base::Timer* timer) {
  DCHECK(timer);
  timer_override_ = timer;
}

void DiscoveryNetworkMonitorMetricObserver::ConfirmDisconnectedToReportMetrics(
    base::TimeTicks disconnect_time) {
  if (last_event_time_) {
    metrics_->RecordTimeBetweenNetworkChangeEvents(disconnect_time -
                                                   *last_event_time_);
  }
  last_event_time_ = disconnect_time;
  metrics_->RecordConnectionType(
      DiscoveryNetworkMonitorConnectionType::kDisconnected);
}

}  // namespace media_router
