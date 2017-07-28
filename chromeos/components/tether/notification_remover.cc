// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/notification_remover.h"

#include "chromeos/components/tether/notification_presenter.h"
#include "chromeos/network/network_state_handler.h"

namespace chromeos {

namespace tether {

NotificationRemover::NotificationRemover(
    HostScanCache* host_scan_cache,
    NetworkStateHandler* network_state_handler,
    NotificationPresenter* notification_presenter)
    : host_scan_cache_(host_scan_cache),
      network_state_handler_(network_state_handler),
      notification_presenter_(notification_presenter) {
  host_scan_cache_->AddObserver(this);
  network_state_handler_->AddObserver(this, FROM_HERE);
}

NotificationRemover::~NotificationRemover() {
  notification_presenter_->RemovePotentialHotspotNotification();
  notification_presenter_->RemoveSetupRequiredNotification();
  notification_presenter_->RemoveConnectionToHostFailedNotification();

  network_state_handler_->RemoveObserver(this, FROM_HERE);
  host_scan_cache_->RemoveObserver(this);
}

void NotificationRemover::OnCacheBecameEmpty() {
  notification_presenter_->RemovePotentialHotspotNotification();
}

void NotificationRemover::DefaultNetworkChanged(const NetworkState* network) {
  if (network)
    notification_presenter_->RemovePotentialHotspotNotification();
}

}  // namespace tether

}  // namespace chromeos
