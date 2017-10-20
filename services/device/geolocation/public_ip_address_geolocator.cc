// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/geolocation/public_ip_address_geolocator.h"

#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/device/geolocation/public_ip_address_location_notifier.h"

namespace device {

// static
void PublicIpAddressGeolocator::Create(
    mojom::GeolocationRequest request,
    const base::WeakPtr<PublicIpAddressLocationNotifier> notifier) {
  mojo::MakeStrongBinding(std::make_unique<PublicIpAddressGeolocator>(notifier),
                          std::move(request));
}

PublicIpAddressGeolocator::PublicIpAddressGeolocator(
    const base::WeakPtr<PublicIpAddressLocationNotifier> notifier)
    : notifier_(notifier), weak_ptr_factory_(this) {}

PublicIpAddressGeolocator::~PublicIpAddressGeolocator() {}

void PublicIpAddressGeolocator::QueryNextPosition(
    QueryNextPositionCallback callback) {
  // TODO(amoylan): Real implementation: query next position after latest time.
  if (notifier_) {
    location_update_subscription_ = notifier_->AddLocationUpdateCallback(
        base::BindRepeating(&PublicIpAddressGeolocator::OnLocationUpdate,
                            weak_ptr_factory_.GetWeakPtr()));
    mojom::GeopositionPtr position = mojom::Geoposition::New();
    position->latitude = 23.0f;
    position->longitude = 80.0f;
    std::move(callback).Run(std::move(position));
  }
}

// Low/high accuracy toggle is ignored by this implementation.
void PublicIpAddressGeolocator::SetHighAccuracy(bool /* high_accuracy */) {}

void PublicIpAddressGeolocator::OnLocationUpdate(const Geoposition& position) {
  // TODO(amoylan): Update last_updated_time and invoke callback.
}

}  // namespace device
