// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/geolocation/public_ip_address_location_notifier.h"

#include "device/geolocation/wifi_data.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace device {

namespace {
// Time to wait before issuing a network geolocation request in response to
// network change notification. Network changes tend to occur in clusters.
constexpr base::TimeDelta kNetworkChangeReactionDelay =
    base::TimeDelta::FromMinutes(5);
}  // namespace

PublicIpAddressLocationNotifier::PublicIpAddressLocationNotifier(
    GeolocationProvider::RequestContextProducer request_context_producer,
    const std::string& api_key)
    : network_changed_since_last_request_(true),
      api_key_(api_key),
      request_context_producer_(request_context_producer),
      weak_ptr_factory_(this) {
  // Subscribe to notifications of changes in network configuration.
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

PublicIpAddressLocationNotifier::~PublicIpAddressLocationNotifier() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void PublicIpAddressLocationNotifier::QueryNextPositionAfterTimestamp(
    base::Time timestamp,
    QueryNextPositionCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // If a network location request is in flight, wait.
  if (network_location_request_) {
    callbacks_.push_back(std::move(callback));
    return;
  }

  // If a network change has occured since we last made a request, start a
  // request and wait.
  if (network_changed_since_last_request_) {
    callbacks_.push_back(std::move(callback));
    MakeNetworkLocationRequest();
    return;
  }

  // If the cached geoposition is newer, provide it immediately.
  if (latest_geoposition_.has_value() &&
      latest_geoposition_->timestamp > timestamp) {
    std::move(callback).Run(*latest_geoposition_);
    return;
  }

  // The cached geoposition is not newer, so wait.
  callbacks_.push_back(std::move(callback));
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

  // Post a cancelable task to react to this network change after a reasonable
  // delay, so that we only react once if multiple network changes occur in a
  // short span of time.
  react_to_network_change_closure_.Reset(base::Bind(
      &PublicIpAddressLocationNotifier::ReactToNetworkChange, GetWeakPtr()));
  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, react_to_network_change_closure_.callback(),
      kNetworkChangeReactionDelay);
}

void PublicIpAddressLocationNotifier::ReactToNetworkChange() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  network_changed_since_last_request_ = true;

  // Invalidate the cached recent position.
  latest_geoposition_.reset();

  // If any clients are waiting, start a request.
  // (This will cancel any previous request, which is OK.)
  if (!callbacks_.empty())
    MakeNetworkLocationRequest();
}

void PublicIpAddressLocationNotifier::MakeNetworkLocationRequest() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  network_changed_since_last_request_ = false;
  // Obtain URL request context using provided producer callback, then continue
  // request in MakeNetworkLocationRequestWithRequestContext.
  request_context_producer_.Run(base::BindOnce(
      &PublicIpAddressLocationNotifier::MakeNetworkLocationRequestWithContext,
      GetWeakPtr()));
}

void PublicIpAddressLocationNotifier::MakeNetworkLocationRequestWithContext(
    scoped_refptr<net::URLRequestContextGetter> context_getter) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!context_getter)
    return;

  network_location_request_ = std::make_unique<NetworkLocationRequest>(
      std::move(context_getter), api_key_,
      base::BindRepeating(
          &PublicIpAddressLocationNotifier::OnNetworkLocationResponse,
          GetWeakPtr()));

  const net::PartialNetworkTrafficAnnotationTag partial_traffic_annotation =
      net::DefinePartialNetworkTrafficAnnotation("public_ip_geolocation",
                                                 "network_location_request",
                                                 R"(
      semantics {
        sender: "Public IP-address geolocation provider"
      }
      policy {
        setting:
          "Separate settings for individual features. "
          "For language detection: Settings -> Languages "
          "Language -> disable 'Offer to transate ...'. "
        chrome_policy {
          TranslateEnabled {
            TranslateEnabled: false
          }
        }
      })");

  // Send an empty request (empty WifiData).
  network_location_request_->MakeRequest(WifiData(), base::Time::Now(),
                                         partial_traffic_annotation);
}

void PublicIpAddressLocationNotifier::OnNetworkLocationResponse(
    const Geoposition& position,
    const bool server_error,
    const WifiData& /* wifi_data */) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (server_error) {
    network_changed_since_last_request_ = true;
  } else {
    latest_geoposition_ = base::make_optional(position);
    // Notify all clients.
    for (QueryNextPositionCallback& callback : callbacks_)
      std::move(callback).Run(position);
    callbacks_.clear();
  }
  network_location_request_.reset();
}

}  // namespace device
