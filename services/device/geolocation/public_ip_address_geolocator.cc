// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/geolocation/public_ip_address_geolocator.h"

#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/device/geolocation/public_ip_address_location_notifier.h"

namespace device {

PublicIpAddressGeolocator::PublicIpAddressGeolocator(
    mojom::GeolocationRequest request,
    const net::PartialNetworkTrafficAnnotationTag tag,
    PublicIpAddressLocationNotifier* const notifier)
    : last_updated_timestamp_(),
      notifier_(notifier),
      binding_(this, std::move(request)),
      network_traffic_annotation_tag_(
          std::make_unique<const net::PartialNetworkTrafficAnnotationTag>(tag)),
      weak_ptr_factory_(this) {}

PublicIpAddressGeolocator::~PublicIpAddressGeolocator() {}

void PublicIpAddressGeolocator::QueryNextPosition(
    QueryNextPositionCallback callback) {
  if (query_next_position_callback_) {
    mojo::ReportBadMessage(
        "Overlapping calls to QueryNextPosition are prohibited.");
    binding_.Close();
    return;
  }

  if (!notifier_)
    return;

  // Request the next position after the latest one we received.
  notifier_->QueryNextPosition(
      last_updated_timestamp_, *network_traffic_annotation_tag_,
      base::BindOnce(&PublicIpAddressGeolocator::OnPositionUpdate,
                     base::Unretained(this)));

  // Retain the callback to use if/when we get a new position.
  query_next_position_callback_ = std::move(callback);
}

// Low/high accuracy toggle is ignored by this implementation.
void PublicIpAddressGeolocator::SetHighAccuracy(bool /* high_accuracy */) {}

void PublicIpAddressGeolocator::OnPositionUpdate(
    const mojom::Geoposition& position) {
  last_updated_timestamp_ = position.timestamp;
  std::move(query_next_position_callback_).Run(position.Clone());
}

}  // namespace device
