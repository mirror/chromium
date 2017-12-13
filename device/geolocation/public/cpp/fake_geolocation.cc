// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "device/geolocation/public/cpp/fake_geolocation.h"
#include "device/geolocation/public/cpp/geoposition.h"
#include "services/device/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/service_context.h"

namespace device {

ScopedGeolocationOverrider::ScopedGeolocationOverrider(
    const mojom::Geoposition& position) {
  OverrideGeolocation(position);
}

ScopedGeolocationOverrider::ScopedGeolocationOverrider(double latitude,
                                                       double longitude) {
  mojom::Geoposition position;
  position.latitude = latitude;
  position.longitude = longitude;
  position.altitude = 0.;
  position.accuracy = 0.;
  position.timestamp = base::Time::Now();

  OverrideGeolocation(position);
}

ScopedGeolocationOverrider::~ScopedGeolocationOverrider() {
  service_manager::ServiceContext::ClearGlobalBindersForTesting(
      mojom::kServiceName);
}

void ScopedGeolocationOverrider::OverrideGeolocation(
    const mojom::Geoposition& position) {
  geolocation_context_ = std::make_unique<FakeGeolocationContext>(position);
  service_manager::ServiceContext::SetGlobalBinderForTesting(
      mojom::kServiceName, mojom::GeolocationContext::Name_,
      base::BindRepeating(&FakeGeolocationContext::BindForOverrideService,
                          base::Unretained(geolocation_context_.get())));
}

void ScopedGeolocationOverrider::UpdateLocation(
    const mojom::Geoposition& position) {
  geolocation_context_->UpdateLocation(position);
}

void ScopedGeolocationOverrider::UpdateLocation(double latitude,
                                                double longitude) {
  mojom::Geoposition position;
  position.latitude = latitude;
  position.longitude = longitude;
  position.altitude = 0.;
  position.accuracy = 0.;
  position.timestamp = base::Time::Now();

  UpdateLocation(position);
}

FakeGeolocationContext::FakeGeolocationContext(
    const mojom::Geoposition& position)
    : position_(position) {
  position_.valid = false;
  if (ValidateGeoposition(position_))
    position_.valid = true;
}

FakeGeolocationContext::~FakeGeolocationContext() {}

void FakeGeolocationContext::UpdateLocation(
    const mojom::Geoposition& position) {
  position_ = position;

  position_.valid = false;
  if (ValidateGeoposition(position_))
    position_.valid = true;

  for (auto& impl : impls_) {
    impl->UpdateLocation(position_);
  }
}

const mojom::Geoposition& FakeGeolocationContext::GetGeoposition() const {
  if (!override_position_.is_null())
    return *override_position_;

  return position_;
}

void FakeGeolocationContext::BindForOverrideService(
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle handle,
    const service_manager::BindSourceInfo& source_info) {
  context_bindings_.AddBinding(
      this, mojom::GeolocationContextRequest(std::move(handle)));
}

void FakeGeolocationContext::BindGeolocation(
    mojom::GeolocationRequest request) {
  impls_.push_back(std::make_unique<FakeGeolocation>(std::move(request), this));
}

void FakeGeolocationContext::SetOverride(mojom::GeopositionPtr geoposition) {
  override_position_ = std::move(geoposition);
  if (override_position_.is_null())
    return;

  override_position_->valid = false;
  if (ValidateGeoposition(*override_position_))
    override_position_->valid = true;

  for (auto& impl : impls_) {
    impl->UpdateLocation(*override_position_);
  }
}

void FakeGeolocationContext::ClearOverride() {
  override_position_.reset();
}

FakeGeolocation::FakeGeolocation(mojom::GeolocationRequest request,
                                 const FakeGeolocationContext* context)
    : context_(context), has_new_position_(true), binding_(this) {
  binding_.Bind(std::move(request));
}

FakeGeolocation::~FakeGeolocation() {}

void FakeGeolocation::UpdateLocation(const mojom::Geoposition& position) {
  has_new_position_ = true;
  if (!position_callback_.is_null()) {
    std::move(position_callback_).Run(position.Clone());
    has_new_position_ = false;
  }
}

void FakeGeolocation::QueryNextPosition(QueryNextPositionCallback callback) {
  // Pending callbacks might be overrided.
  position_callback_ = std::move(callback);

  if (has_new_position_) {
    std::move(position_callback_).Run(context_->GetGeoposition().Clone());
    has_new_position_ = false;
    return;
  }
}

void FakeGeolocation::SetHighAccuracy(bool high_accuracy) {}

}  // namespace device
