// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/geolocation/geolocation_context.h"

#include <utility>

#include "base/memory/ptr_util.h"
#include "device/geolocation/geolocation_impl.h"

namespace device {

GeolocationContext::GeolocationContext() {}

GeolocationContext::~GeolocationContext() {}

void GeolocationContext::CreateService(
    const service_manager::BindSourceInfo& source_info,
    mojom::GeolocationRequest request) {
  GeolocationImpl* service = new GeolocationImpl(std::move(request), this);
  services_.push_back(base::WrapUnique<GeolocationImpl>(service));
  if (geoposition_override_)
    service->SetOverride(*geoposition_override_.get());
  else
    service->StartListeningForUpdates();
}

void GeolocationContext::ServiceHadConnectionError(GeolocationImpl* service) {
  auto it = std::find_if(services_.begin(), services_.end(),
                         [service](const std::unique_ptr<GeolocationImpl>& s) {
                           return service == s.get();
                         });
  DCHECK(it != services_.end());
  services_.erase(it);
}

void GeolocationContext::SetOverride(std::unique_ptr<Geoposition> geoposition) {
  geoposition_override_.swap(geoposition);
  for (auto& service : services_) {
    service->SetOverride(*geoposition_override_.get());
  }
}

void GeolocationContext::ClearOverride() {
  for (auto& service : services_) {
    service->ClearOverride();
  }
}

}  // namespace device
