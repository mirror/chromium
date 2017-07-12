// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/geolocation/geolocation_service_impl.h"

#include "content/public/browser/permission_manager.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_frame_host.h"
#include "device/geolocation/geolocation_context.h"

namespace device {

GeolocationServiceImpl::GeolocationServiceImpl(
    GeolocationContext* geolocation_context,
    content::PermissionManager* permission_manager,
    content::RenderFrameHost* render_frame_host)
    : geolocation_context_(geolocation_context),
      permission_manager_(permission_manager),
      render_frame_host_(render_frame_host) {
  DCHECK(geolocation_context);
  DCHECK(permission_manager);
}

GeolocationServiceImpl::~GeolocationServiceImpl(){};

void GeolocationServiceImpl::Bind(
    const service_manager::BindSourceInfo& source_info,
    mojom::GeolocationServiceRequest request) {
  binding_set_.AddBinding(this, std::move(request));
}

void GeolocationServiceImpl::CreateGeolocation(
    mojo::InterfaceRequest<device::mojom::Geolocation> request,
    bool user_gesture) {
  permission_manager_->RequestPermission(
      content::PermissionType::GEOLOCATION, render_frame_host_,
      render_frame_host_->GetLastCommittedOrigin().GetURL(), user_gesture,
      base::Bind(&GeolocationServiceImpl::CreateGeolocationWithPermissionStatus,
                 base::Unretained(this), base::Passed(&request)));
}

void GeolocationServiceImpl::CreateGeolocationWithPermissionStatus(
    mojom::GeolocationRequest request,
    blink::mojom::PermissionStatus permission_status) {
  if (permission_status != blink::mojom::PermissionStatus::GRANTED)
    return;

  geolocation_context_->Bind(std::move(request));
}

}  // namespace device
