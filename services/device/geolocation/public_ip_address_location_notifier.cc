// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/geolocation/public_ip_address_location_notifier.h"

namespace device {

// Time to wait before issuing a network geolocation request in response to
// network change notification. Network changes tend to occur in clusters.
constexpr base::TimeDelta kNetworkChangeReactionDelay =
    base::TimeDelta::FromMinutes(1);

void foo() {
  // TODO(amoylan): Make this a member of PublicIpAddressLocationNotifier and
  // use it to tear down the NetworkLocationRequest, if the number of clients is
  // zero. Then revive it again if the clients goes back up > 0.
}

PublicIpAddressLocationNotifier::PublicIpAddressLocationNotifier(
    GeolocationProvider::RequestContextProducer request_context_producer,
    const std::string& api_key)
    : api_key_(api_key),
      request_context_producer_(request_context_producer),
      weak_ptr_factory_(this) {
  // Subscribe to notifications of changes in network configuration.
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);

  // Obtain URL request context using provided producer callback, then continue
  // initialization in OnRequestContextResponse.
  request_context_producer_.Run(
      base::BindOnce(&PublicIpAddressLocationNotifier::OnRequestContextResponse,
                     GetWeakPtr()));

  callbacks_.set_removal_callback(base::Bind(&foo));
}

void PublicIpAddressLocationNotifier::OnRequestContextResponse(
    scoped_refptr<net::URLRequestContextGetter> context_getter) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (context_getter != nullptr) {
    network_location_request_ = std::make_unique<NetworkLocationRequest>(
        std::move(context_getter), api_key_,
        base::BindRepeating(
            &PublicIpAddressLocationNotifier::OnNetworkLocationResponse,
            GetWeakPtr()));
  }
  // TODO(amoylan): Make initial request if clients are waiting.
}

PublicIpAddressLocationNotifier::~PublicIpAddressLocationNotifier() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

PublicIpAddressLocationNotifier::LocationUpdateSubscription
PublicIpAddressLocationNotifier::AddLocationUpdateCallback(
    LocationUpdateCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  LocationUpdateSubscription subscription = callbacks_.Add(std::move(callback));
  // TODO(amoylan): Immediately call if appropriate.
  return subscription;
}

base::WeakPtr<PublicIpAddressLocationNotifier>
PublicIpAddressLocationNotifier::GetWeakPtr() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return weak_ptr_factory_.GetWeakPtr();
}

void PublicIpAddressLocationNotifier::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Ignore destructive signals.
  if (type == net::NetworkChangeNotifier::CONNECTION_NONE)
    return;

  // Post a cancelable task to react to this network change after a short delay,
  // so that we only react once if multiple network changes occur in a short
  // span of time.
  react_to_network_change_closure_.Reset(base::Bind(
      &PublicIpAddressLocationNotifier::ReactToNetworkChange, GetWeakPtr()));
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, react_to_network_change_closure_.callback(),
      kNetworkChangeReactionDelay);
}

void PublicIpAddressLocationNotifier::ReactToNetworkChange() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(amoylan): 1. Update last-network-change-timestamp.
  //                2. If any clients are waiting, kick off a network
  //                   geolocation request.
}

void PublicIpAddressLocationNotifier::OnNetworkLocationResponse(
    const Geoposition& position,
    const bool server_error,
    const WifiData& /* wifi_data */) {}

}  // namespace device
