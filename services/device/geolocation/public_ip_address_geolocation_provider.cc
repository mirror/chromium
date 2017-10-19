// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/geolocation/public_ip_address_geolocation_provider.h"

#include "services/device/geolocation/public_ip_address_geolocator.h"

namespace device {

PublicIpAddressGeolocationProvider::PublicIpAddressGeolocationProvider() {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

PublicIpAddressGeolocationProvider::~PublicIpAddressGeolocationProvider() {}

void PublicIpAddressGeolocationProvider::Initialize(
    GeolocationProvider::RequestContextProducer request_context_producer,
    const std::string& api_key) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  public_ip_address_location_notifier_ =
      std::make_unique<PublicIpAddressLocationNotifier>(
          request_context_producer, api_key);
}

bool PublicIpAddressGeolocationProvider::IsInitialized() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return public_ip_address_location_notifier_ != nullptr;
}

void PublicIpAddressGeolocationProvider::Bind(
    mojom::PublicIpAddressGeolocationProviderRequest request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());
  binding_set_.AddBinding(this, std::move(request));
}

void PublicIpAddressGeolocationProvider::CreateGeolocation(
    mojom::GeolocationRequest request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(IsInitialized());
  PublicIpAddressGeolocator::Create(
      std::move(request), public_ip_address_location_notifier_->GetWeakPtr());
}

}  // namespace device
