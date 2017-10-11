// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/geolocation/public_ip_address_location_notifier.h"

namespace device {

PublicIpAddressLocationNotifier::PublicIpAddressLocationNotifier(
    base::SequencedTaskRunner* const task_runner,
    GeolocationProvider::RequestContextProducer request_context_producer,
    const std::string& api_key)
    : api_key_(api_key), request_context_producer_(request_context_producer) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
  // TODO(amoylan): Replace Unretained(this) everywhere with a weak ptr factory.
  task_runner->PostTask(FROM_HERE,
                        base::BindOnce(&PublicIpAddressLocationNotifier::Start,
                                       base::Unretained(this)));
}

void PublicIpAddressLocationNotifier::Start() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Subscribe to notifications of changes in network configuration.
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);

  // Obtain URL request context using provided producer callback, then continue
  // initialization in OnRequestContextResponse.
  request_context_producer_.Run(
      base::Bind(&PublicIpAddressLocationNotifier::OnRequestContextResponse,
                 base::Unretained(this)));
}

void PublicIpAddressLocationNotifier::OnRequestContextResponse(
    scoped_refptr<net::URLRequestContextGetter> context_getter) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (context_getter != nullptr) {
    network_location_request_ = std::make_unique<NetworkLocationRequest>(
        std::move(context_getter), api_key_,
        base::BindRepeating(
            &PublicIpAddressLocationNotifier::OnNetworkLocationResponse,
            base::Unretained(this)));
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

void PublicIpAddressLocationNotifier::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Ignore destructive signals.
  if (type == net::NetworkChangeNotifier::CONNECTION_NONE)
    return;
  // TODO(amoylan): Start cancelable task to perform an update in one minute.
}

void PublicIpAddressLocationNotifier::OnNetworkLocationResponse(
    const Geoposition& position,
    const bool server_error,
    const WifiData& /* wifi_data */) {}

}  // namespace device
