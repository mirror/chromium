// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/generic_sensor/sensor_provider_proxy_impl.h"
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

namespace {
using SensorType = device::mojom::SensorType;

std::vector<PermissionType> SensorTypeToPermissionTypes(SensorType type) {
  std::vector<PermissionType> permission_types;
  switch (type) {
    case SensorType::AMBIENT_LIGHT:
      permission_types.push_back(PermissionType::AMBIENT_LIGHT_SENSOR);
      break;
    case SensorType::PROXIMITY:
      NOTREACHED();
      break;
    case SensorType::ACCELEROMETER:
      permission_types.push_back(PermissionType::ACCELEROMETER);
      break;
    case SensorType::LINEAR_ACCELERATION:
      NOTREACHED();
      break;
    case SensorType::GYROSCOPE:
      permission_types.push_back(PermissionType::GYROSCOPE);
      break;
    case SensorType::MAGNETOMETER:
      permission_types.push_back(PermissionType::MAGNETOMETER);
      break;
    case SensorType::PRESSURE:
      NOTREACHED();
      break;
    case SensorType::ABSOLUTE_ORIENTATION:
      permission_types.push_back(PermissionType::ACCELEROMETER);
      permission_types.push_back(PermissionType::GYROSCOPE);
      permission_types.push_back(PermissionType::MAGNETOMETER);
      break;
    case SensorType::RELATIVE_ORIENTATION:
      permission_types.push_back(PermissionType::ACCELEROMETER);
      permission_types.push_back(PermissionType::GYROSCOPE);
      break;
  }
  return permission_types;
}
}  // namespace

SensorProviderProxyImpl::SensorProviderProxyImpl(
    RenderFrameHost* render_frame_host)
    : WebContentsObserver(WebContents::FromRenderFrameHost(render_frame_host)),
      render_frame_host_(render_frame_host) {}

SensorProviderProxyImpl::~SensorProviderProxyImpl() = default;

// static
void SensorProviderProxyImpl::Create(
    RenderFrameHost* render_frame_host,
    const service_manager::BindSourceInfo& source_info,
    device::mojom::SensorProviderRequest request) {
  mojo::MakeStrongBinding(
      base::WrapUnique(new SensorProviderProxyImpl(render_frame_host)),
      std::move(request));
}

void SensorProviderProxyImpl::GetSensor(
    device::mojom::SensorType type,
    device::mojom::SensorRequest sensor_request,
    GetSensorCallback callback) {
  ServiceManagerConnection* connection =
      ServiceManagerConnection::GetForProcess();

  if (!connection || !CheckPermission(type)) {
    std::move(callback).Run(nullptr, nullptr);
    return;
  }

  if (!sensor_provider_) {
    connection->GetConnector()->BindInterface(
        device::mojom::kServiceName, mojo::MakeRequest(&sensor_provider_));
    sensor_provider_.set_connection_error_handler(
        base::Bind(&device::mojom::SensorProviderPtr::reset,
                   base::Unretained(&sensor_provider_)));
  }
  sensor_provider_->GetSensor(type, std::move(sensor_request),
                              std::move(callback));
}

bool SensorProviderProxyImpl::CheckPermission(
    device::mojom::SensorType type) const {
  if (!web_contents())
    return false;
  BrowserContext* browser_context = web_contents()->GetBrowserContext();
  if (!browser_context)
    return false;
  PermissionManager* permission_manager =
      browser_context->GetPermissionManager();
  if (!permission_manager)
    return false;

  const GURL& embedding_origin =
      web_contents()->GetMainFrame()->GetLastCommittedURL();
  const GURL& requesting_origin = render_frame_host_->GetLastCommittedURL();

  blink::mojom::PermissionStatus permission_status =
      blink::mojom::PermissionStatus::DENIED;
  for (PermissionType permission_type : SensorTypeToPermissionTypes(type)) {
    permission_status = permission_manager->GetPermissionStatus(
        permission_type, requesting_origin, embedding_origin);
    if (permission_status != blink::mojom::PermissionStatus::GRANTED)
      break;
  }
  return permission_status == blink::mojom::PermissionStatus::GRANTED;
}

}  // namespace content
