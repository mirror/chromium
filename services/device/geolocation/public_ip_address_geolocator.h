// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GEOLOCATION_PUBLIC_IP_ADDRESS_GEOLOCATOR_H_
#define DEVICE_GEOLOCATION_PUBLIC_IP_ADDRESS_GEOLOCATOR_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/device/geolocation/public_ip_address_location_notifier.h"
#include "services/device/public/interfaces/public_ip_address_geolocator.mojom.h"

namespace device {

class PublicIpAddressLocationNotifier;

// Implements the PublicIpAddressGeolocator Mojo interface.
class PublicIpAddressGeolocator : public mojom::PublicIpAddressGeolocator {
 public:
  // Creates a PublicIpAddresGeolocator strongly-bound to the specified
  // |request| and subscribed to the specified |notifier|.
  static void Create(mojom::PublicIpAddressGeolocatorRequest request,
                     PublicIpAddressLocationNotifier* notifier);

  PublicIpAddressGeolocator(PublicIpAddressLocationNotifier* notifier);
  ~PublicIpAddressGeolocator() override;

 private:
  // mojom::PublicIpAddressGeolocator:
  void QueryNextPosition(QueryNextPositionCallback callback) override;

  // The callback passed to QueryNextPosition.
  QueryNextPositionCallback position_callback_;

  // Subscription to PublicIpAddressLocationNotifier callback list.
  PublicIpAddressLocationNotifier::LocationUpdateSubscription
      location_update_subscription_;

  // Callback to register with PublicIpAddressLocationNotifier.
  void OnLocationUpdate(const Geoposition& position);

  // TODO(amoylan): Replace with timestamp of latest known position.
  mojom::Geoposition current_position_;

  // TODO(amoylan): Store a pointer to the notifier passed in to constructor.

  DISALLOW_COPY_AND_ASSIGN(PublicIpAddressGeolocator);
};

}  // namespace device

#endif  // DEVICE_GEOLOCATION_PUBLIC_IP_ADDRESS_GEOLOCATOR_H_
