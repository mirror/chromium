// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/host_scan_cache.h"
#include "chromeos/network/network_state_handler_observer.h"

namespace chromeos {

class NetworkStateHandler;

namespace tether {

class NotificationPresenter;

// Removes notifications when there are no potential hotspots nearby, or when
// the device connects to a network.
class NotificationRemover : public HostScanCache::Observer,
                            public NetworkStateHandlerObserver {
 public:
  NotificationRemover(HostScanCache* host_scan_cache,
                      NetworkStateHandler* network_state_handler,
                      NotificationPresenter* notification_presenter);

  ~NotificationRemover() override;

  // HostScanCache::Observer:
  void OnCacheBecameEmpty() override;

  // NetworkStateHandlerObserver:
  void DefaultNetworkChanged(const NetworkState* network) override;

 private:
  HostScanCache* host_scan_cache_;
  NetworkStateHandler* network_state_handler_;
  NotificationPresenter* notification_presenter_;

  DISALLOW_COPY_AND_ASSIGN(NotificationRemover);
};

}  // namespace tether

}  // namespace chromeos
