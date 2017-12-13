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
class FakeGeolocationContext;

// A helper class which owns a FakeGeolocationContext by which the geolocation
// is overrided to a given position or latitude and longitude values.
// The FakeGeolocationContext override the binder of Device Service by
// service_manager::ServiceContext::SetGlobalBinderForTesting().
// The override of the geolocation implementation will be in effect for the
// duration of this object's lifetime.
class ScopedGeolocationOverrider {
 public:
  explicit ScopedGeolocationOverrider(const mojom::Geoposition& position);
  ScopedGeolocationOverrider(double latitude, double longitude);
  ~ScopedGeolocationOverrider();
  void OverrideGeolocation(const mojom::Geoposition& position);
  void UpdateLocation(const mojom::Geoposition& position);
  void UpdateLocation(double latitude, double longitude);

 private:
  std::unique_ptr<FakeGeolocationContext> geolocation_context_;
};

// This class is a fake implementation of GeolocationContext and Geolocation
// mojo interfaces for those tests which want to set an override geoposition
// value and verify their code where there are geolocation mojo calls.
// Test clients are supposed to use the ScopedGeolocationOverrider helper class
// rather than use this class directly.
class FakeGeolocationContext : public mojom::GeolocationContext {
 public:
  explicit FakeGeolocationContext(const mojom::Geoposition& position);
  ~FakeGeolocationContext() override;

  void UpdateLocation(const mojom::Geoposition& position);
  const mojom::Geoposition& GetGeoposition() const;

  void BindForOverrideService(
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle handle,
      const service_manager::BindSourceInfo& source_info);

  // mojom::GeolocationContext implementation:
  void BindGeolocation(mojom::GeolocationRequest request) override;
  void SetOverride(mojom::GeopositionPtr geoposition) override;
  void ClearOverride() override;

 private:
  mojom::Geoposition position_;
  mojom::GeopositionPtr override_position_;
  std::vector<std::unique_ptr<FakeGeolocation>> impls_;
  mojo::BindingSet<mojom::GeolocationContext> context_bindings_;
};

class FakeGeolocation : public mojom::Geolocation {
 public:
  FakeGeolocation(mojom::GeolocationRequest request,
                  const FakeGeolocationContext* context);
  ~FakeGeolocation() override;

  void UpdateLocation(const mojom::Geoposition& position);

  // mojom::Geolocation implementation:
  void QueryNextPosition(QueryNextPositionCallback callback) override;
  void SetHighAccuracy(bool high_accuracy) override;

 private:
  const FakeGeolocationContext* context_;
  bool has_new_position_;
  QueryNextPositionCallback position_callback_;
  mojo::Binding<mojom::Geolocation> binding_;
};

}  // namespace device

#endif  // DEVICE_GEOLOCATION_PUBLIC_CPP_FAKE_GEOLOCATION_
