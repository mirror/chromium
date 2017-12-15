// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_interface_binders.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "content/browser/background_fetch/background_fetch_service_impl.h"
#include "content/browser/dedicated_worker/dedicated_worker_host.h"
#include "content/browser/locks/lock_manager.h"
#include "content/browser/notifications/platform_notification_context_impl.h"
#include "content/browser/payments/payment_manager.h"
#include "content/browser/permissions/permission_service_context.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/browser/storage_partition_impl.h"
#include "content/browser/websockets/websocket_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_interface_broker.h"
#include "content/public/browser/web_interface_broker_builder.h"
#include "content/public/browser/web_interface_filter.h"
#include "services/device/public/interfaces/constants.mojom.h"
#include "services/device/public/interfaces/vibration_manager.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/shape_detection/public/interfaces/barcodedetection.mojom.h"
#include "services/shape_detection/public/interfaces/constants.mojom.h"
#include "services/shape_detection/public/interfaces/facedetection_provider.mojom.h"
#include "services/shape_detection/public/interfaces/textdetection.mojom.h"
#include "third_party/WebKit/public/platform/modules/notifications/notification_service.mojom.h"
#include "url/origin.h"

namespace content {
namespace {

void ForwardFrameRequestToEmbedder(const std::string& interface_name,
                                   mojo::ScopedMessagePipeHandle interface_pipe,
                                   RenderFrameHost* frame,
                                   mojo::ReportBadMessageCallback) {
  GetContentClient()->browser()->BindInterfaceRequestFromFrame(
      frame, interface_name, std::move(interface_pipe));
}

void ForwardWorkerRequestToEmbedder(
    WebContextType,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe,
    RenderProcessHost* process_host,
    const url::Origin& origin,
    mojo::ReportBadMessageCallback) {
  GetContentClient()->browser()->BindInterfaceRequestFromWorker(
      process_host, origin, interface_name, std::move(interface_pipe));
}

// Create a WebInterfaceBroker with content-layer renderer-exposed interfaces
// registered.  Each registered interface binder is exposed to all
// renderer-hosted execution context types (document/frame, dedicated worker,
// shared worker and service worker) unless they apply an appropriate
// WebInterfaceFilter. For interface requests from frames, binders registered on
// the frame itself override binders registered here.
std::unique_ptr<WebInterfaceBroker> CreateContentWebInterfaceBroker() {
  auto builder = WebInterfaceBrokerBuilder::Create(
      base::BindRepeating(&ForwardWorkerRequestToEmbedder),
      base::BindRepeating(&ForwardFrameRequestToEmbedder));

  builder->AddInterface<shape_detection::mojom::BarcodeDetection>(
      shape_detection::mojom::kServiceName, {});
  builder->AddInterface<shape_detection::mojom::FaceDetectionProvider>(
      shape_detection::mojom::kServiceName, {});
  builder->AddInterface<shape_detection::mojom::TextDetection>(
      shape_detection::mojom::kServiceName, {});
  builder->AddInterface<device::mojom::VibrationManager>(
      device::mojom::kServiceName, {});
  builder->AddInterface(
      base::BindRepeating(
          [](blink::mojom::WebSocketRequest request, RenderFrameHost* host) {
            WebSocketManager::CreateWebSocket(host->GetProcess()->GetID(),
                                              host->GetRoutingID(),
                                              std::move(request));
          }),
      base::Bind([](blink::mojom::WebSocketRequest request,
                    RenderProcessHost* host, const url::Origin& origin) {
        WebSocketManager::CreateWebSocket(host->GetID(), MSG_ROUTING_NONE,
                                          std::move(request));
      }),
      {});
  builder->AddInterface(
      base::Bind([](payments::mojom::PaymentManagerRequest request,
                    RenderProcessHost* host, const url::Origin& origin) {
        static_cast<StoragePartitionImpl*>(host->GetStoragePartition())
            ->GetPaymentAppContext()
            ->CreatePaymentManager(std::move(request));
      }),
      CreateContextTypeInterfaceFilter(
          {WebContextType::kFrame, WebContextType::kServiceWorker}));
  builder->AddInterface(
      base::Bind([](blink::mojom::PermissionServiceRequest request,
                    RenderProcessHost* host, const url::Origin& origin) {
        static_cast<RenderProcessHostImpl*>(host)
            ->permission_service_context()
            .CreateServiceForWorker(std::move(request), origin);
      }),
      {});
  builder->AddInterface(
      base::BindRepeating([](blink::mojom::LockManagerRequest request,
                             RenderProcessHost* host,
                             const url::Origin& origin) {
        static_cast<StoragePartitionImpl*>(host->GetStoragePartition())
            ->GetLockManager()
            ->CreateService(std::move(request));
      }),
      {});
  builder->AddInterface(
      base::BindRepeating(&CreateDedicatedWorkerHostFactory),
      CreateContextTypeInterfaceFilter({WebContextType::kFrame}));
  builder->AddInterface(
      base::Bind([](blink::mojom::NotificationServiceRequest request,
                    RenderProcessHost* host, const url::Origin& origin) {
        static_cast<StoragePartitionImpl*>(host->GetStoragePartition())
            ->GetPlatformNotificationContext()
            ->CreateService(host->GetID(), origin, std::move(request));
      }),
      {});
  builder->AddInterface(
      base::BindRepeating(&BackgroundFetchServiceImpl::Create), {});

  return builder->Build();
}

}  // namespace

WebInterfaceBroker& GetWebInterfaceBroker() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  CR_DEFINE_STATIC_LOCAL(std::unique_ptr<WebInterfaceBroker>, broker,
                         (CreateContentWebInterfaceBroker()));
  return *broker;
}

}  // namespace content
