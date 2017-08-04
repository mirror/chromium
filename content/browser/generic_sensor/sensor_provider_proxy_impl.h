// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_GENERIC_SENSOR_SENSOR_PROVIDER_PROXY_IMPL_H_
#define CONTENT_BROWSER_GENERIC_SENSOR_SENSOR_PROVIDER_PROXY_IMPL_H_

#include "content/public/browser/web_contents_observer.h"
#include "services/device/public/interfaces/sensor_provider.mojom.h"

namespace content {

class RenderFrameHost;

class SensorProviderProxyImpl final : public device::mojom::SensorProvider,
                                      public WebContentsObserver {
 public:
  static void Create(RenderFrameHost* render_frame_host,
                     device::mojom::SensorProviderRequest request);

  ~SensorProviderProxyImpl() override;

 private:
  explicit SensorProviderProxyImpl(RenderFrameHost* render_frame_host);

  // SensorProvider implementation.
  void GetSensor(device::mojom::SensorType type,
                 device::mojom::SensorRequest sensor_request,
                 GetSensorCallback callback) override;

  bool CheckPermission(device::mojom::SensorType type) const;

  RenderFrameHost* render_frame_host_;
  device::mojom::SensorProviderPtr sensor_provider_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_GENERIC_SENSOR_SENSOR_PROVIDER_PROXY_IMPL_H_
