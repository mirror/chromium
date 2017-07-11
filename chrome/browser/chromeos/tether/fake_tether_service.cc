// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/tether/fake_tether_service.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/chromeos/net/tether_notification_presenter.h"
#include "chromeos/network/network_connect.h"
#include "components/cryptauth/remote_device.h"
#include "components/proximity_auth/logging/logging.h"
#include "ui/message_center/message_center.h"
constexpr char kTetherGuidPrefix[] = "tether-guid-";
constexpr char kTetherNamePrefix[] = "tether";
constexpr char kCarrier[] = "FakeCarrier";

FakeTetherService::FakeTetherService(
    Profile* profile,
    chromeos::PowerManagerClient* power_manager_client,
    chromeos::SessionManagerClient* session_manager_client,
    cryptauth::CryptAuthService* cryptauth_service,
    chromeos::NetworkStateHandler* network_state_handler)
    : TetherService(profile,
                    power_manager_client,
                    session_manager_client,
                    cryptauth_service,
                    network_state_handler) {}

void FakeTetherService::StartTetherIfEnabled() {
  if (GetTetherTechnologyState() !=
      chromeos::NetworkStateHandler::TechnologyState::TECHNOLOGY_ENABLED) {
    return;
  }

  for (int i = 0; i < num_tether_networks_; ++i) {
    network_state_handler()->AddTetherNetworkState(
        kTetherGuidPrefix + std::to_string(i),
        kTetherNamePrefix + std::to_string(i), kCarrier,
        100 /* battery_percentage */, 75 /* signal_strength */,
        false /* has_connected_to_host */);
  }
  auto notification_presenter =
      base::MakeUnique<chromeos::tether::TetherNotificationPresenter>(
          profile_, message_center::MessageCenter::Get(),
          chromeos::NetworkConnect::Get());
  PA_LOG(INFO) << "FOR REGAN: FORCE OPEN WIFI VIEW!";
  cryptauth::RemoteDevice device;
  device.public_key = "publicKey0";  //+ std::to_string(i);
  device.name = "ReganPixel";

  // Get Network state associated with the device (i.e ReganPixel)
  const chromeos::NetworkState* lol =
      network_state_handler()->GetNetworkStateFromGuid(kTetherGuidPrefix +
                                                       std::to_string(0));

  // pass device and corresponding network state to the function
  notification_presenter->NotifyPotentialHotspotNearby(device, lol);
}

void FakeTetherService::StopTether() {
  for (int i = 0; i < num_tether_networks_; ++i) {
    network_state_handler()->RemoveTetherNetworkState(kTetherGuidPrefix + i);
  }
}

bool FakeTetherService::HasSyncedTetherHosts() const {
  return true;
}
