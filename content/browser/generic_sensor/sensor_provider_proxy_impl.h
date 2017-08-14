// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_GENERIC_SENSOR_SENSOR_PROVIDER_PROXY_IMPL_H_
#define CONTENT_BROWSER_GENERIC_SENSOR_SENSOR_PROVIDER_PROXY_IMPL_H_

#include "content/public/browser/web_contents_observer.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/device/public/interfaces/sensor_provider.mojom.h"

namespace content {

class PermissionManager;
class RenderFrameHost;

class SensorProviderProxyImpl final : public device::mojom::SensorProvider {
 public:
  SensorProviderProxyImpl(PermissionManager* permission_manager,
                          RenderFrameHost* render_frame_host);
  ~SensorProviderProxyImpl() override;

  void Bind(device::mojom::SensorProviderRequest request);

 private:
  // SensorProvider implementation.
  void GetSensor(device::mojom::SensorType type,
                 device::mojom::SensorRequest sensor_request,
                 GetSensorCallback callback) override;

  bool CheckPermission(device::mojom::SensorType type) const;

  mojo::BindingSet<device::mojom::SensorProvider> binding_set_;
  PermissionManager* permission_manager_;
  RenderFrameHost* render_frame_host_;
  device::mojom::SensorProviderPtr sensor_provider_;

  DISALLOW_COPY_AND_ASSIGN(SensorProviderProxyImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_GENERIC_SENSOR_SENSOR_PROVIDER_PROXY_IMPL_H_
