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
    : location_update_subscription_(notifier->AddLocationUpdateCallback(
          base::BindRepeating(&PublicIpAddressGeolocator::OnLocationUpdate,
                              base::Unretained(this)))) {
  // TODO(amoylan): Don't subscribe here. Instead subscribe with each new
  //                QueryNextPosition call.
  // TODO(amoylan): I don't think this can be base::Unretained: This class will
  //                self-destruct if the Mojo connection goes down. Need weakptr
  //                factory.
}

PublicIpAddressGeolocator::~PublicIpAddressGeolocator() {}

void PublicIpAddressGeolocator::QueryNextPosition(
    const QueryNextPositionCallback callback) {}

void PublicIpAddressGeolocator::OnLocationUpdate(const Geoposition& position) {}

}  // namespace device
