// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DISCOVERY_NETWORK_MONITOR_METRIC_OBSERVER_H_
#define CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DISCOVERY_NETWORK_MONITOR_METRIC_OBSERVER_H_

#include "chrome/browser/media/router/discovery/discovery_network_monitor.h"

#include "base/optional.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/media/router/discovery/discovery_network_monitor_metrics.h"

namespace media_router {

class DiscoveryNetworkMonitorMetricObserver final
    : public DiscoveryNetworkMonitor::Observer {
 public:
  DiscoveryNetworkMonitorMetricObserver(
      std::unique_ptr<base::TickClock> tick_clock,
      std::unique_ptr<DiscoveryNetworkMonitorMetrics> metrics);
  ~DiscoveryNetworkMonitorMetricObserver();

  // DiscoveryNetworkMonitor::Observer implementation.
  void OnNetworksChanged(const std::string&) override;

  void SetTimerForTest(base::Timer* timer);

 private:
  void ConfirmDisconnectedToReportMetrics(base::TimeTicks disconnect_time);

  const base::Timer& timer_() const {
    return timer_override_ ? static_cast<const base::Timer&>(*timer_override_)
                           : disconnect_timer_;
  }
  base::Timer& timer_() {
    return timer_override_ ? *timer_override_ : disconnect_timer_;
  }

  std::unique_ptr<base::TickClock> tick_clock_;
  std::unique_ptr<DiscoveryNetworkMonitorMetrics> metrics_;
  std::string network_id_;
  base::Optional<base::TimeTicks> last_event_time_;
  // TODO: maybe inject this but can't verify it's OneShot via Timer interface.
  base::OneShotTimer disconnect_timer_;
  base::Timer* timer_override_ = nullptr;
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_DISCOVERY_DISCOVERY_NETWORK_MONITOR_METRIC_OBSERVER_H_
