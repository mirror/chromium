// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_GEOLOCATION_GEOLOCATION_SERVICE_IMPL_H_
#define CONTENT_BROWSER_GEOLOCATION_GEOLOCATION_SERVICE_IMPL_H_

#include "content/common/content_export.h"
#include "device/geolocation/public/interfaces/geolocation.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace blink {
namespace mojom {
enum class PermissionStatus;
}
}  // namespace blink

namespace device {
class GeolocationContext;
}

namespace service_manager {
struct BindSourceInfo;
}

namespace content {
class RenderFrameHost;
class PermissionManager;

class CONTENT_EXPORT GeolocationServiceImpl
    : public device::mojom::GeolocationService {
 public:
  GeolocationServiceImpl(device::GeolocationContext* geolocation_context,
                         PermissionManager* permission_manager,
                         RenderFrameHost* render_frame_host);
  ~GeolocationServiceImpl() override;

  // Binds to the GeolocationService.
  void Bind(const service_manager::BindSourceInfo& source_info,
            device::mojom::GeolocationServiceRequest request);

  // Creates the Geolocation Service.
  void CreateGeolocation(device::mojom::GeolocationRequest request,
                         bool user_gesture) override;

 private:
  // Creates the Geolocation Service.
  void CreateGeolocationWithPermissionStatus(
      device::mojom::GeolocationRequest request,
      blink::mojom::PermissionStatus permission_status);

  device::GeolocationContext* geolocation_context_;
  PermissionManager* permission_manager_;
  RenderFrameHost* render_frame_host_;

  mojo::BindingSet<device::mojom::GeolocationService> binding_set_;

  DISALLOW_COPY_AND_ASSIGN(GeolocationServiceImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_GEOLOCATION_GEOLOCATION_SERVICE_IMPL_H_
