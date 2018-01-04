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

using blink::mojom::PermissionStatus;
using device::mojom::SensorType;

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

namespace {

std::vector<PermissionType> GetPermissionTypes(SensorType type) {
  switch (type) {
    case SensorType::AMBIENT_LIGHT:
      return {PermissionType::AMBIENT_LIGHT_SENSOR};
    case SensorType::ACCELEROMETER:
    case SensorType::LINEAR_ACCELERATION:
      return {PermissionType::ACCELEROMETER};
    case SensorType::GYROSCOPE:
      return {PermissionType::GYROSCOPE};
    case SensorType::MAGNETOMETER:
      return {PermissionType::MAGNETOMETER};
    case SensorType::ABSOLUTE_ORIENTATION_EULER_ANGLES:
    case SensorType::ABSOLUTE_ORIENTATION_QUATERNION:
      return {PermissionType::ACCELEROMETER, PermissionType::GYROSCOPE,
              PermissionType::MAGNETOMETER};
    case SensorType::RELATIVE_ORIENTATION_EULER_ANGLES:
    case SensorType::RELATIVE_ORIENTATION_QUATERNION:
      return {PermissionType::ACCELEROMETER, PermissionType::GYROSCOPE};
    default:
      NOTREACHED() << "Unknown sensor type " << type;
      return {};
  }
}

}  // namespace

void SensorProviderProxyImpl::GetSensor(
    device::mojom::SensorType type,
    GetSensorCallback callback) {
  const std::vector<PermissionType>& permission_types =
      GetPermissionTypes(type);
  if (permission_types.empty()) {
    std::move(callback).Run(nullptr);
    return;
  }
  const GURL& requesting_origin = render_frame_host_->GetLastCommittedURL();
  auto permissions_callback = base::BindRepeating(
      &SensorProviderProxyImpl::OnRequestPermissionsResponse,
      weak_factory_.GetWeakPtr(), type, base::Passed(std::move(callback)));
  permission_manager_->RequestPermissions(
      permission_types, render_frame_host_, requesting_origin,
      false /*user_gesture*/, permissions_callback);
}

void SensorProviderProxyImpl::OnRequestPermissionsResponse(
    device::mojom::SensorType type,
    GetSensorCallback callback,
    const std::vector<PermissionStatus>& result) {
  ServiceManagerConnection* connection =
      ServiceManagerConnection::GetForProcess();

  if (!connection ||
      std::any_of(result.begin(), result.end(), [](PermissionStatus status) {
        return status != PermissionStatus::GRANTED;
      })) {
    std::move(callback).Run(nullptr);
    return;
  }

  if (!sensor_provider_) {
    connection->GetConnector()->BindInterface(
        device::mojom::kServiceName, mojo::MakeRequest(&sensor_provider_));
    sensor_provider_.set_connection_error_handler(base::BindOnce(
        &SensorProviderProxyImpl::OnConnectionError, base::Unretained(this)));
  }
  sensor_provider_->GetSensor(type, std::move(callback));
}

void SensorProviderProxyImpl::OnConnectionError() {
  // Close all the upstream bindings to notify them of this failure as the
  // GetSensorCallbacks will never be called.
  binding_set_.CloseAllBindings();
  sensor_provider_.reset();
  // Invalidate pending callbacks.
  weak_factory_.InvalidateWeakPtrs();
}

}  // namespace content
