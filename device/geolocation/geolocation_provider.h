// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_GEOLOCATION_GEOLOCATION_PROVIDER_H_
#define DEVICE_GEOLOCATION_GEOLOCATION_PROVIDER_H_

#include <memory>

#include "base/callback_list.h"
#include "device/geolocation/geolocation_export.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/device/public/interfaces/geoposition.mojom.h"

namespace device {

// TODO(ke.he@intel.com): With the proceeding of the servicification of
// geolocation, the geolocation core will be moved into //services/device and as
// a internal part of Device Service. This geolocation_provider.h will also be
// removed.
class GeolocationProvider {
 public:
  DEVICE_GEOLOCATION_EXPORT static GeolocationProvider* GetInstance();


  // Overrides the current location for testing.
  //
  // Overrides the location for automation/testing. Suppresses any further
  // updates from the actual providers and sends an update with the overridden
  // position to all registered clients.
  //
  // Do not use this function in unit tests. The function instantiates the
  // singleton geolocation stack in the background and manipulates it to report
  // a fake location. Neither step can be undone, breaking unit test isolation
  // (crbug.com/125931).
  virtual void OverrideLocationForTesting(
      const mojom::Geoposition& position) = 0;

 protected:
  virtual ~GeolocationProvider() {}
};

}  // namespace device

#endif  // DEVICE_GEOLOCATION_GEOLOCATION_PROVIDER_H_
