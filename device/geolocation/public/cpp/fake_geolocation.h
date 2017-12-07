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
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/bind_source_info.h"

namespace device {

class FakeGeolocation;

// This class is a fake implementation of GeolocationContext and Geolocation
// mojo interfaces for those tests which want to set an override geoposition
// value and verify their code where there are geolocation mojo calls.
// Test clients create a FakeGeolocationContext instance first, then they can
// 1) override the binder of their Connector by
// service_manager::Connector::TestApi::OverrideBinderForTesting(). and 2)
// override the binder of Device Service by
// service_manager::ServiceContext::SetGlobalBinderForTesting(). Note that the
// FakeGeolocationContext instance must be alive during test clients call the
// geolocation mojo interfaces.
class FakeGeolocationContext : public mojom::GeolocationContext {
 public:
  explicit FakeGeolocationContext(const mojom::Geoposition& position);
  ~FakeGeolocationContext() override;

  void UpdateLocation(const mojom::Geoposition& position);
  const mojom::Geoposition& GetGeoposition() const;

  void BindForOverrideConnector(mojo::ScopedMessagePipeHandle handle);
  void BindForOverrideService(
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle handle,
      const service_manager::BindSourceInfo& source_info);

  // device::mojom::GeolocationContext implementation:
  void BindGeolocation(device::mojom::GeolocationRequest request) override;
  void SetOverride(device::mojom::GeopositionPtr geoposition) override;
  void ClearOverride() override;

 private:
  mojom::Geoposition position_;
  mojom::GeopositionPtr override_position_;
  std::vector<std::unique_ptr<FakeGeolocation>> impls_;
  mojo::BindingSet<device::mojom::GeolocationContext> context_bindings_;
};

class FakeGeolocation : public mojom::Geolocation {
 public:
  FakeGeolocation(mojom::GeolocationRequest request,
                  const FakeGeolocationContext* context);
  ~FakeGeolocation() override;

  void UpdateLocation(const mojom::Geoposition& position);

  // device::mojom::Geolocation implementation:
  void QueryNextPosition(QueryNextPositionCallback callback) override;
  void SetHighAccuracy(bool high_accuracy) override;

 private:
  const FakeGeolocationContext* context_;
  bool has_new_position_;
  QueryNextPositionCallback position_callback_;
  mojo::Binding<device::mojom::Geolocation> binding_;
};

// A helper class which owns a FakeGeolocationContext by which the geolocation
// is overrided to given latitude and longitude values. Test clients must make
// sure this instance be alive during they call the geolocation mojo interfaces.
class ScopedGeolocationOverrider {
 public:
  explicit ScopedGeolocationOverrider(double latitude, double longitude);
  ~ScopedGeolocationOverrider();
  void UpdateLocation(double latitude, double longitude);

 private:
  std::unique_ptr<FakeGeolocationContext> geolocation_context_;
};

}  // namespace device

#endif  // DEVICE_GEOLOCATION_PUBLIC_CPP_FAKE_GEOLOCATION_
