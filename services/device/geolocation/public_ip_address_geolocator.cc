// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/geolocation/public_ip_address_geolocator.h"

#include "services/device/geolocation/public_ip_address_location_notifier.h"

namespace device {

// static
void PublicIpAddressGeolocator::Create(
    mojom::GeolocationRequest request,
    const base::WeakPtr<PublicIpAddressLocationNotifier> notifier) {
  // Create strong binding of request to a new implementation object.
  const mojo::StrongBindingPtr<mojom::Geolocation> binding =
      mojo::MakeStrongBinding(
          std::make_unique<PublicIpAddressGeolocator>(notifier),
          std::move(request));

  // Give the implementation a weak reference to its binding.
  static_cast<PublicIpAddressGeolocator*>(binding->impl())->binding_ = binding;
}

PublicIpAddressGeolocator::PublicIpAddressGeolocator(
    const base::WeakPtr<PublicIpAddressLocationNotifier> notifier)
    : last_updated_timestamp_(), notifier_(notifier), weak_ptr_factory_(this) {}

PublicIpAddressGeolocator::~PublicIpAddressGeolocator() {}

void PublicIpAddressGeolocator::QueryNextPosition(
    QueryNextPositionCallback callback) {
  if (query_next_position_callback_) {
    mojo::ReportBadMessage(
        "Overlapping calls to QueryNextPosition are prohibited.");
    binding_->Close();
  }

  if (!notifier_)
    return;

  // Request the next position after the latest one we received.
  notifier_->QueryNextPositionAfterTimestamp(
      last_updated_timestamp_,
      base::BindOnce(&PublicIpAddressGeolocator::OnPositionUpdate,
                     weak_ptr_factory_.GetWeakPtr()));

  // Retain the callback to use if/when we get a new position.
  query_next_position_callback_ = std::move(callback);
}

// Low/high accuracy toggle is ignored by this implementation.
void PublicIpAddressGeolocator::SetHighAccuracy(bool /* high_accuracy */) {}

void PublicIpAddressGeolocator::OnPositionUpdate(const Geoposition& position) {
  last_updated_timestamp_ = position.timestamp;

  // Convert to mojom::Geoposition and provide result.
  mojom::GeopositionPtr result = mojom::Geoposition::New();
  FillMojomGeoposition(position, result.get());

  std::move(query_next_position_callback_).Run(std::move(result));
}

}  // namespace device
