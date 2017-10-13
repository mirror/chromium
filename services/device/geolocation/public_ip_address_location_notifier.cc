// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/geolocation/public_ip_address_location_notifier.h"

namespace device {

// Time to wait before issuing a network geolocation request in response to
// network change notification. Network changes tend to occur in clusters.
constexpr base::TimeDelta kNetworkChangeReactionDelay =
    base::TimeDelta::FromMinutes(1);

PublicIpAddressLocationNotifier::PublicIpAddressLocationNotifier(
    base::SequencedTaskRunner* const task_runner,
    GeolocationProvider::RequestContextProducer request_context_producer,
    const std::string& api_key)
    : api_key_(api_key),
      request_context_producer_(request_context_producer),
      weak_ptr_factory_(this) {
  DETACH_FROM_SEQUENCE(task_runner_sequence_checker_);

  // Subscribe to notifications of changes in network configuration.
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);

  task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&PublicIpAddressLocationNotifier::BackgroundStart,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PublicIpAddressLocationNotifier::BackgroundStart() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(task_runner_sequence_checker_);

  // Obtain URL request context using provided producer callback, then continue
  // initialization in OnRequestContextResponse.
  request_context_producer_.Run(
      base::BindOnce(&PublicIpAddressLocationNotifier::OnRequestContextResponse,
                     weak_ptr_factory_.GetWeakPtr()));
}

void PublicIpAddressLocationNotifier::OnRequestContextResponse(
    scoped_refptr<net::URLRequestContextGetter> context_getter) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(task_runner_sequence_checker_);
  if (context_getter != nullptr) {
    network_location_request_ = std::make_unique<NetworkLocationRequest>(
        std::move(context_getter), api_key_,
        base::BindRepeating(
            &PublicIpAddressLocationNotifier::OnNetworkLocationResponse,
            weak_ptr_factory_.GetWeakPtr()));

    // Prevent a leak until the destruction story for this class is figured out.
    network_location_request_.reset();
  }
  // TODO(amoylan): Make initial request if clients are waiting.
}

PublicIpAddressLocationNotifier::~PublicIpAddressLocationNotifier() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(creation_sequence_checker_);
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

PublicIpAddressLocationNotifier::LocationUpdateSubscription
PublicIpAddressLocationNotifier::AddLocationUpdateCallback(
    LocationUpdateCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(task_runner_sequence_checker_);
  LocationUpdateSubscription subscription = callbacks_.Add(std::move(callback));
  // TODO(amoylan): Immediately call if appropriate.
  return subscription;
}

void PublicIpAddressLocationNotifier::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(creation_sequence_checker_);

  // Ignore destructive signals.
  if (type == net::NetworkChangeNotifier::CONNECTION_NONE)
    return;

  // TODO(amoylan): Unretained is no good. Either another weakptrfactory is
  // required on a different thread (yuck!) or spin off a small helper class
  // "delayed network change observer" for the main-thread parts (better!).
  react_to_network_change_closure_.Reset(
      base::Bind(&PublicIpAddressLocationNotifier::ReactToNetworkChange,
                 base::Unretained(this)));
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, react_to_network_change_closure_.callback(),
      kNetworkChangeReactionDelay);
}

void PublicIpAddressLocationNotifier::ReactToNetworkChange() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(creation_sequence_checker_);
  // TODO(amoylan): Call this on other sequence.
  // TODO(amoylan): 1. Update last-network-change-timestamp.
  //                2. If any clients are waiting, kick off a network
  //                   geolocation request.
}

void PublicIpAddressLocationNotifier::OnNetworkLocationResponse(
    const Geoposition& position,
    const bool server_error,
    const WifiData& /* wifi_data */) {}

}  // namespace device
