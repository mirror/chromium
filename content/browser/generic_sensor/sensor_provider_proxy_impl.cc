// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/generic_sensor/sensor_provider_proxy_impl.h"

#include <utility>

#include "content/public/browser/browser_context.h"
#include "content/public/browser/permission_manager.h"
#include "content/public/browser/permission_type.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/service_manager_connection.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/device/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace content {

SensorProviderProxyImpl::SensorProviderProxyImpl(
    PermissionManager* permission_manager,
    RenderFrameHost* render_frame_host)
    : permission_manager_(permission_manager),
      render_frame_host_(render_frame_host),
      weak_factory_(this) {
  DCHECK(permission_manager);
  DCHECK(render_frame_host);
}

SensorProviderProxyImpl::~SensorProviderProxyImpl() = default;

void SensorProviderProxyImpl::Bind(
    device::mojom::SensorProviderRequest request) {
  binding_set_.AddBinding(this, std::move(request));
}

void SensorProviderProxyImpl::GetSensor(
    device::mojom::SensorType type,
    GetSensorCallback callback) {

  if (!sensor_provider_) {
    ServiceManagerConnection* connection =
        ServiceManagerConnection::GetForProcess();

    if (!connection) {
      std::move(callback).Run(nullptr);
      return;
    }

    connection->GetConnector()->BindInterface(
        device::mojom::kServiceName, mojo::MakeRequest(&sensor_provider_));
    sensor_provider_.set_connection_error_handler(base::BindOnce(
        &SensorProviderProxyImpl::OnConnectionError, base::Unretained(this)));
  }

  // TODO(shalamov): base::BindOnce should be used (https://crbug.com/714018),
  // however, PermissionManager::RequestPermission enforces use of repeating
  // callback.
  permission_manager_->RequestPermission(
      PermissionType::SENSORS, render_frame_host_,
      render_frame_host_->GetLastCommittedURL(), false,
      base::Bind(&SensorProviderProxyImpl::OnPermissionRequestCompleted,
                 weak_factory_.GetWeakPtr(), type,
                 base::Passed(std::move(callback))));
}

void SensorProviderProxyImpl::OnPermissionRequestCompleted(
    device::mojom::SensorType type,
    GetSensorCallback callback,
    blink::mojom::PermissionStatus status) {
  if (status != blink::mojom::PermissionStatus::GRANTED || !sensor_provider_) {
    std::move(callback).Run(nullptr);
    return;
  }
  sensor_provider_->GetSensor(type, std::move(callback));
}

void SensorProviderProxyImpl::OnConnectionError() {
  // Close all the upstream bindings to notify them of this failure as the
  // GetSensorCallbacks will never be called.
  binding_set_.CloseAllBindings();
  sensor_provider_.reset();
}

}  // namespace content
