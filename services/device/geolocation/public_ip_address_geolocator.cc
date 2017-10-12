// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/geolocation/public_ip_address_geolocator.h"

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/device/geolocation/public_ip_address_location_notifier.h"

namespace device {

// static
void PublicIpAddressGeolocator::Create(
    mojom::PublicIpAddressGeolocatorRequest request,
    PublicIpAddressLocationNotifier* const notifier) {
  mojo::MakeStrongBinding(std::make_unique<PublicIpAddressGeolocator>(notifier),
                          std::move(request));
}

PublicIpAddressGeolocator::PublicIpAddressGeolocator(
    PublicIpAddressLocationNotifier* const notifier)
    : notifier_(notifier), weak_ptr_factory_(this) {}

PublicIpAddressGeolocator::~PublicIpAddressGeolocator() {}

void PublicIpAddressGeolocator::QueryNextPosition(
    QueryNextPositionCallback callback) {
  // TODO(amoylan): Real implementation: query next position after latest time.
  location_update_subscription_ = notifier_->AddLocationUpdateCallback(
      base::BindRepeating(&PublicIpAddressGeolocator::OnLocationUpdate,
                          weak_ptr_factory_.GetWeakPtr()));
  mojom::GeopositionPtr position = mojom::Geoposition::New();
  position->latitude = 4.0f;
  position->longitude = -7.0f;
  std::move(callback).Run(std::move(position));
}

void PublicIpAddressGeolocator::OnLocationUpdate(const Geoposition& position) {}

}  // namespace device
