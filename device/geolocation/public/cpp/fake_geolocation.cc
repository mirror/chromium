// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/geolocation/public/cpp/fake_geolocation.h"

namespace device {

FakeGeolocation::FakeGeolocation(device::mojom::Geoposition& position)
    : binding_context_(this), binding_(this), position_(position) {}

FakeGeolocation::~FakeGeolocation() {}

void FakeGeolocation::Bind(mojo::ScopedMessagePipeHandle handle) {
  binding_context_.Bind(
      device::mojom::GeolocationContextRequest(std::move(handle)));
}

void FakeGeolocation::QueryNextPosition(QueryNextPositionCallback callback) {
  std::move(callback).Run(position_.Clone());
}

void FakeGeolocation::SetHighAccuracy(bool high_accuracy) {}

void FakeGeolocation::BindGeolocation(
    device::mojom::GeolocationRequest request) {
  binding_.Bind(std::move(request));
}

void FakeGeolocation::SetOverride(device::mojom::GeopositionPtr geoposition) {}

void FakeGeolocation::ClearOverride() {}

}  // namespace device
