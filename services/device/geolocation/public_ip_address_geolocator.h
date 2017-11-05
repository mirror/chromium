// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_GEOLOCATION_PUBLIC_IP_ADDRESS_GEOLOCATOR_H_
#define SERVICES_DEVICE_GEOLOCATION_PUBLIC_IP_ADDRESS_GEOLOCATOR_H_

#include "base/macros.h"
#include "base/time/time.h"
#include "device/geolocation/public/interfaces/geolocation.mojom.h"
#include "device/geolocation/public/interfaces/geoposition.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/device/geolocation/public_ip_address_location_notifier.h"

namespace device {

class PublicIpAddressLocationNotifier;

// An implementation of Geolocation that uses only public IP address-based
// geolocation.
class PublicIpAddressGeolocator : public mojom::Geolocation {
 public:
  // Creates a PublicIpAddresGeolocator strongly-bound to the specified
  // |request| and subscribed to the specified |notifier|.
  static void Create(mojom::GeolocationRequest request,
                     base::WeakPtr<PublicIpAddressLocationNotifier> notifier);

  PublicIpAddressGeolocator(
      base::WeakPtr<PublicIpAddressLocationNotifier> notifier);
  ~PublicIpAddressGeolocator() override;

 private:
  // mojom::Geolocation:
  void QueryNextPosition(QueryNextPositionCallback callback) override;
  void SetHighAccuracy(bool high_accuracy) override;

  // Callback to register with PublicIpAddressLocationNotifier.
  void OnPositionUpdate(const mojom::Geoposition& position);

  // The callback passed to QueryNextPosition.
  QueryNextPositionCallback query_next_position_callback_;

  // Timestamp of latest Geoposition this client received.
  base::Time last_updated_timestamp_;

  // Notifier to ask for IP-geolocation updates.
  const base::WeakPtr<PublicIpAddressLocationNotifier> notifier_;

  // A weak reference to the StrongBinding owning this object.
  mojo::StrongBindingPtr<mojom::Geolocation> binding_;

  // Weak references to |this| for posted tasks.
  base::WeakPtrFactory<PublicIpAddressGeolocator> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PublicIpAddressGeolocator);
};

}  // namespace device

#endif  // SERVICES_DEVICE_GEOLOCATION_PUBLIC_IP_ADDRESS_GEOLOCATOR_H_
