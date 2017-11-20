// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GEOLOCATION_PUBLIC_CPP_FAKE_GEOLOCATION_
#define DEVICE_GEOLOCATION_PUBLIC_CPP_FAKE_GEOLOCATION_

#include "base/bind.h"
#include "device/geolocation/public/interfaces/geolocation.mojom.h"
#include "device/geolocation/public/interfaces/geolocation_context.mojom.h"
#include "device/geolocation/public/interfaces/geoposition.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace device {

class FakeGeolocation : public device::mojom::GeolocationContext,
                        public device::mojom::Geolocation {
 public:
  explicit FakeGeolocation(device::mojom::Geoposition& position);
  ~FakeGeolocation() override;

  void Bind(mojo::ScopedMessagePipeHandle handle);

  // device::mojom::Geolocation implementation:
  void QueryNextPosition(QueryNextPositionCallback callback) override;
  void SetHighAccuracy(bool high_accuracy) override;

  // device::mojom::GeolocationContext implementation:
  void BindGeolocation(device::mojom::GeolocationRequest request) override;
  void SetOverride(device::mojom::GeopositionPtr geoposition) override;
  void ClearOverride() override;

 private:
  mojo::Binding<device::mojom::GeolocationContext> binding_context_;
  mojo::Binding<device::mojom::Geolocation> binding_;
  device::mojom::Geoposition position_;
};

}  // namespace device

#endif  // DEVICE_GEOLOCATION_PUBLIC_CPP_FAKE_GEOLOCATION_
