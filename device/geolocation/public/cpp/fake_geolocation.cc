// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "device/geolocation/public/cpp/fake_geolocation.h"
#include "device/geolocation/public/cpp/geoposition.h"

namespace device {

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

const mojom::Geoposition FakeGeolocationContext::GetGeoposition() const {
  return position_;
}

void FakeGeolocationContext::BindForOverrideConnector(
    mojo::ScopedMessagePipeHandle handle) {
  context_bindings_.AddBinding(
      this, mojom::GeolocationContextRequest(std::move(handle)));
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

void FakeGeolocationContext::SetOverride(mojom::GeopositionPtr geoposition) {}
void FakeGeolocationContext::ClearOverride() {}

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
