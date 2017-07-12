// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_GEOLOCATION_SERVICE_IMPL_H_
#define CONTENT_BROWSER_GEOLOCATION_SERVICE_IMPL_H_

#include "content/common/content_export.h"
#include "device/geolocation/public/interfaces/geolocation.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace blink {
namespace mojom {
enum class PermissionStatus;
}
}  // namespace blink

namespace content {
class RenderFrameHost;
class PermissionManager;
}  // namespace content

namespace service_manager {
struct BindSourceInfo;
}

namespace device {

class GeolocationContext;

class CONTENT_EXPORT GeolocationServiceImpl : public mojom::GeolocationService {
 public:
  GeolocationServiceImpl(GeolocationContext* geolocation_context,
                         content::PermissionManager* permission_manager,
                         content::RenderFrameHost* render_frame_host);
  ~GeolocationServiceImpl() override;

  // Binds to the GeolocationService.
  void Bind(const service_manager::BindSourceInfo& source_info,
            mojom::GeolocationServiceRequest request);

  // Creates the Geolocation Service.
  void CreateGeolocation(mojom::GeolocationRequest request,
                         bool user_gesture) override;

 private:
  // Creates the Geolocation Service.
  void CreateGeolocationWithPermissionStatus(
      mojom::GeolocationRequest request,
      blink::mojom::PermissionStatus permission_status);

  GeolocationContext* geolocation_context_;
  content::PermissionManager* permission_manager_;
  content::RenderFrameHost* render_frame_host_;

  // mojo::BindingSet<mojom::WakeLock, std::unique_ptr<bool>> binding_set_;
  mojo::BindingSet<mojom::GeolocationService> binding_set_;

  DISALLOW_COPY_AND_ASSIGN(GeolocationServiceImpl);
};

}  // namespace device

#endif  // CONTENT_BROWSER_GEOLOCATION_SERVICE_IMPL_H_
