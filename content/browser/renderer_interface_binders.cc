// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_interface_binders.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "build/build_config.h"
#include "cc/base/switches.h"
#include "content/browser/background_fetch/background_fetch_service_impl.h"
#include "content/browser/dedicated_worker/dedicated_worker_host.h"
#include "content/browser/frame_host/input/input_injector_impl.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/image_capture/image_capture_impl.h"
#include "content/browser/installedapp/installed_app_provider_impl_default.h"
#include "content/browser/keyboard_lock/keyboard_lock_service_impl.h"
#include "content/browser/locks/lock_manager.h"
#include "content/browser/media/session/media_session_service_impl.h"
#include "content/browser/notifications/platform_notification_context_impl.h"
#include "content/browser/payments/payment_manager.h"
#include "content/browser/permissions/permission_service_context.h"
#include "content/browser/renderer_host/media/media_devices_dispatcher_host.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/browser/shared_worker/shared_worker_connector_impl.h"
#include "content/browser/storage_partition_impl.h"
#include "content/browser/websockets/websocket_manager.h"
#include "content/network/restricted_cookie_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_context_type.h"
#include "content/public/browser/web_interface_broker.h"
#include "content/public/browser/web_interface_broker_builder.h"
#include "content/public/browser/web_interface_filter.h"
#include "content/public/browser/webvr_service_provider.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "device/vr/vr_service.mojom.h"
#include "media/mojo/interfaces/remoting.mojom.h"
#include "media/mojo/services/media_metrics_provider.h"
#include "services/device/public/interfaces/constants.mojom.h"
#include "services/device/public/interfaces/sensor_provider.mojom.h"
#include "services/device/public/interfaces/vibration_manager.mojom.h"
#include "services/resource_coordinator/public/cpp/frame_resource_coordinator.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/shape_detection/public/interfaces/barcodedetection.mojom.h"
#include "services/shape_detection/public/interfaces/constants.mojom.h"
#include "services/shape_detection/public/interfaces/facedetection_provider.mojom.h"
#include "services/shape_detection/public/interfaces/textdetection.mojom.h"
#include "third_party/WebKit/public/platform/modules/notifications/notification_service.mojom.h"
#include "url/origin.h"

#if defined(OS_ANDROID)
#include "content/browser/media/android/media_player_renderer.h"
#include "media/base/audio_renderer_sink.h"
#include "media/base/video_renderer_sink.h"
#include "media/mojo/services/mojo_renderer_service.h"  // nogncheck
#endif

namespace content {
namespace {

void ForwardFrameRequestToEmbedder(const std::string& interface_name,
                                   mojo::ScopedMessagePipeHandle interface_pipe,
                                   RenderFrameHost* frame,
                                   mojo::ReportBadMessageCallback) {
  GetContentClient()->browser()->BindInterfaceRequestFromFrame(
      frame, interface_name, std::move(interface_pipe));
}

void ForwardAssociatedFrameRequestToEmbedder(
    const std::string& interface_name,
    mojo::ScopedInterfaceEndpointHandle interface_pipe,
    RenderFrameHost* frame,
    mojo::ReportBadMessageCallback) {
  GetContentClient()->browser()->BindAssociatedInterfaceRequestFromFrame(
      frame, interface_name, &interface_pipe);
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

template <typename Interface>
void ForwardToRenderFrameHostImplImpl(
    const base::RepeatingCallback<void(RenderFrameHostImpl*,
                                       mojo::InterfaceRequest<Interface>)>&
        callback,
    mojo::InterfaceRequest<Interface> request,
    RenderFrameHost* frame) {
  return callback.Run(static_cast<RenderFrameHostImpl*>(frame),
                      std::move(request));
}

template <typename Interface>
base::RepeatingCallback<void(mojo::InterfaceRequest<Interface>,
                             RenderFrameHost*)>
ForwardToRenderFrameHostImpl(
    base::RepeatingCallback<void(RenderFrameHostImpl*,
                                 mojo::InterfaceRequest<Interface>)> callback) {
  return base::BindRepeating(&ForwardToRenderFrameHostImplImpl<Interface>,
                             std::move(callback));
}

void GetRestrictedCookieManager(
    network::mojom::RestrictedCookieManagerRequest request,
    RenderProcessHost* render_process_host,
    const url::Origin& origin) {
  StoragePartition* storage_partition =
      render_process_host->GetStoragePartition();
  mojom::NetworkContext* network_context =
      storage_partition->GetNetworkContext();
  uint32_t render_process_id = render_process_host->GetID();
  network_context->GetRestrictedCookieManager(
      std::move(request), render_process_id, MSG_ROUTING_NONE);
}

#if BUILDFLAG(ENABLE_MEDIA_REMOTING)
// RemoterFactory that delegates Create() calls to the ContentBrowserClient.
//
// Since Create() could be called at any time, perhaps by a stray task being run
// after a RenderFrameHost has been destroyed, the RemoterFactoryImpl uses the
// process/routing IDs as a weak reference to the RenderFrameHostImpl.
class RemoterFactoryImpl final : public media::mojom::RemoterFactory {
 public:
  RemoterFactoryImpl(int process_id, int routing_id)
      : process_id_(process_id), routing_id_(routing_id) {}

  static void Bind(media::mojom::RemoterFactoryRequest request,
                   RenderFrameHost* frame) {
    mojo::MakeStrongBinding(
        std::make_unique<RemoterFactoryImpl>(frame->GetProcess()->GetID(),
                                             frame->GetRoutingID()),
        std::move(request));
  }

 private:
  void Create(media::mojom::RemotingSourcePtr source,
              media::mojom::RemoterRequest request) final {
    if (auto* host = RenderFrameHostImpl::FromID(process_id_, routing_id_)) {
      GetContentClient()->browser()->CreateMediaRemoter(host, std::move(source),
                                                        std::move(request));
    }
  }

  const int process_id_;
  const int routing_id_;

  DISALLOW_COPY_AND_ASSIGN(RemoterFactoryImpl);
};
#endif  // BUILDFLAG(ENABLE_MEDIA_REMOTING)

#if defined(OS_ANDROID)
void CreateMediaPlayerRenderer(media::mojom::RendererRequest request,
                               RenderFrameHost* frame) {
  std::unique_ptr<MediaPlayerRenderer> renderer =
      std::make_unique<MediaPlayerRenderer>(
          frame->GetProcess()->GetID(), frame->GetRoutingID(),
          WebContents::FromRenderFrameHost(frame));

  // base::Unretained is safe here because the lifetime of the MediaPlayerRender
  // is tied to the lifetime of the MojoRendererService.
  media::MojoRendererService::InitiateSurfaceRequestCB surface_request_cb =
      base::BindRepeating(&MediaPlayerRenderer::InitiateScopedSurfaceRequest,
                          base::Unretained(renderer.get()));

  media::MojoRendererService::Create(
      nullptr,  // CDMs are not supported.
      nullptr,  // Manages its own audio_sink.
      nullptr,  // Does not use video_sink. See StreamTextureWrapper instead.
      std::move(renderer), surface_request_cb, std::move(request));
}
#endif  // defined(OS_ANDROID)

// Create a WebInterfaceBroker with content-layer renderer-exposed interfaces
// registered.  Each registered interface binder is exposed to all
// renderer-hosted execution context types (document/frame, dedicated worker,
// shared worker and service worker) unless they apply an appropriate
// WebInterfaceFilter. For interface requests from frames, binders registered on
// the frame itself override binders registered here.
std::unique_ptr<WebInterfaceBroker> CreateContentWebInterfaceBroker() {
  auto builder = WebInterfaceBrokerBuilder::Create(
      base::BindRepeating(&ForwardWorkerRequestToEmbedder),
      base::BindRepeating(&ForwardFrameRequestToEmbedder),
      base::BindRepeating(&ForwardAssociatedFrameRequestToEmbedder));

  builder->AddInterface<shape_detection::mojom::BarcodeDetection>(
      CreateAlwaysAllowFilter(), shape_detection::mojom::kServiceName);
  builder->AddInterface<shape_detection::mojom::FaceDetectionProvider>(
      CreateAlwaysAllowFilter(), shape_detection::mojom::kServiceName);
  builder->AddInterface<shape_detection::mojom::TextDetection>(
      CreateAlwaysAllowFilter(), shape_detection::mojom::kServiceName);
  builder->AddInterface<device::mojom::VibrationManager>(
      CreateAlwaysAllowFilter(), device::mojom::kServiceName);
  builder->AddInterface<device::mojom::SensorProvider>(
      CreateFilterBundle(
          CreateContextTypeInterfaceFilter({WebContextType::kFrame}),
          CreatePermissionInterfaceFilter(PermissionType::SENSORS)),
      device::mojom::kServiceName);

  builder->AddInterface(
      CreateAlwaysAllowFilter(),
      base::BindRepeating(
          [](blink::mojom::WebSocketRequest request, RenderFrameHost* host) {
            WebSocketManager::CreateWebSocket(host->GetProcess()->GetID(),
                                              host->GetRoutingID(),
                                              std::move(request));
          }),
      base::BindRepeating([](blink::mojom::WebSocketRequest request,
                             RenderProcessHost* host,
                             const url::Origin& origin) {
        WebSocketManager::CreateWebSocket(host->GetID(), MSG_ROUTING_NONE,
                                          std::move(request));
      }));
  builder->AddInterface(
      CreateContextTypeInterfaceFilter(
          {WebContextType::kFrame, WebContextType::kServiceWorker}),
      base::BindRepeating([](payments::mojom::PaymentManagerRequest request,
                             RenderProcessHost* host,
                             const url::Origin& origin) {
        static_cast<StoragePartitionImpl*>(host->GetStoragePartition())
            ->GetPaymentAppContext()
            ->CreatePaymentManager(std::move(request));
      }));
  builder->AddInterface(
      CreateAlwaysAllowFilter(),
      base::BindRepeating([](blink::mojom::PermissionServiceRequest request,
                             RenderFrameHost* frame) {
        static_cast<RenderFrameHostImpl*>(frame)
            ->permission_service_context()
            .CreateService(std::move(request));
      }),
      base::BindRepeating([](blink::mojom::PermissionServiceRequest request,
                             RenderProcessHost* host,
                             const url::Origin& origin) {
        static_cast<RenderProcessHostImpl*>(host)
            ->permission_service_context()
            .CreateServiceForWorker(std::move(request), origin);
      }));
  builder->AddInterface(
      CreateAlwaysAllowFilter(),
      base::BindRepeating([](blink::mojom::LockManagerRequest request,
                             RenderProcessHost* host,
                             const url::Origin& origin) {
        static_cast<StoragePartitionImpl*>(host->GetStoragePartition())
            ->GetLockManager()
            ->CreateService(std::move(request), origin);
      }));
  builder->AddInterface(
      CreateContextTypeInterfaceFilter({WebContextType::kFrame}),
      base::BindRepeating(&CreateDedicatedWorkerHostFactory));
  builder->AddInterface(
      CreateAlwaysAllowFilter(),
      base::BindRepeating([](blink::mojom::NotificationServiceRequest request,
                             RenderProcessHost* host,
                             const url::Origin& origin) {
        static_cast<StoragePartitionImpl*>(host->GetStoragePartition())
            ->GetPlatformNotificationContext()
            ->CreateService(host->GetID(), origin, std::move(request));
      }));
  builder->AddInterface(
      CreateAlwaysAllowFilter(),
      base::BindRepeating(&BackgroundFetchServiceImpl::Create));

  builder->AddInterface(CreateAlwaysAllowFilter(),
                        ForwardToRenderFrameHostImpl(base::BindRepeating(
                            &RenderFrameHostImpl::BindWakeLockRequest)));

  builder->AddInterface(
      CreateAlwaysAllowFilter(),
      ForwardToRenderFrameHostImpl(base::BindRepeating(
          &RenderFrameHostImpl::BindPresentationServiceRequest)));

  builder->AddInterface(CreateAlwaysAllowFilter(),
                        base::BindRepeating(&MediaSessionServiceImpl::Create));

  builder->AddInterface(
      CreateAlwaysAllowFilter(),
      ForwardToRenderFrameHostImpl(base::BindRepeating(base::IgnoreResult(
          &RenderFrameHostImpl::CreateWebBluetoothService))));

  builder->AddInterface(CreateAlwaysAllowFilter(),
                        ForwardToRenderFrameHostImpl(base::BindRepeating(
                            &RenderFrameHostImpl::CreateUsbDeviceManager)));

  builder->AddInterface(CreateAlwaysAllowFilter(),
                        ForwardToRenderFrameHostImpl(base::BindRepeating(
                            &RenderFrameHostImpl::CreateUsbChooserService)));

  builder->AddInterface(
      CreateAlwaysAllowFilter(),
      ForwardToRenderFrameHostImpl(base::BindRepeating(
          &RenderFrameHostImpl::BindMediaInterfaceFactoryRequest)));

  builder->AddInterface(
      CreateAlwaysAllowFilter(),
      base::BindRepeating([](mojom::SharedWorkerConnectorRequest request,
                             RenderFrameHost* frame) {
        SharedWorkerConnectorImpl::Create(frame->GetProcess()->GetID(),
                                          frame->GetRoutingID(),
                                          std::move(request));
      }));

  builder->AddInterface(
      CreateAlwaysAllowFilter(),
      base::BindRepeating(&WebvrServiceProvider::BindWebvrService));

  if (RendererAudioOutputStreamFactoryContextImpl::UseMojoFactories()) {
    builder->AddInterface(
        CreateAlwaysAllowFilter(),
        ForwardToRenderFrameHostImpl(base::BindRepeating(
            &RenderFrameHostImpl::CreateAudioOutputStreamFactory)));
  }

  if (resource_coordinator::IsResourceCoordinatorEnabled()) {
    builder->AddInterface(
        CreateAlwaysAllowFilter(),
        base::BindRepeating(
            [](resource_coordinator::mojom::FrameCoordinationUnitRequest
                   request,
               RenderFrameHost* render_frame_host) {
              static_cast<RenderFrameHostImpl*>(render_frame_host)
                  ->GetFrameResourceCoordinator()
                  ->AddBinding(std::move(request));
            }));
  }

  builder->AddInterface(
      CreateAlwaysAllowFilter(),
      base::BindRepeating(&KeyboardLockServiceImpl::CreateMojoService));

  builder->AddInterface(
      CreateContextTypeInterfaceFilter({WebContextType::kFrame}),
      base::BindRepeating([](media::mojom::ImageCaptureRequest request,
                             RenderProcessHost*, const url::Origin&) {
        ImageCaptureImpl::Create(std::move(request));
      }));

  builder->AddInterface(
      CreateAlwaysAllowFilter(),
      base::BindRepeating([](media::mojom::MediaMetricsProviderRequest request,
                             RenderFrameHost* frame) {
        // Only save decode stats when on-the-record.
        media::MediaMetricsProvider::Create(
            frame->GetSiteInstance()->GetBrowserContext()->IsOffTheRecord()
                ? nullptr
                : frame->GetSiteInstance()
                      ->GetBrowserContext()
                      ->GetVideoDecodePerfHistory(),
            std::move(request));
      }));

  builder->AddInterface(
      CreateCommandLineSwitchFilter(cc::switches::kEnableGpuBenchmarking),
      ForwardToRenderFrameHostImpl(
          base::BindRepeating(&InputInjectorImpl::Create)));

  builder->AddInterface(CreateCommandLineSwitchFilter(
                            switches::kEnableExperimentalWebPlatformFeatures),
                        base::BindRepeating(GetRestrictedCookieManager));

#if defined(OS_ANDROID)
  builder->AddInterface<device::mojom::NFC>(
      CreateFeatureFilter(features::kWebNfc),
      ForwardToRenderFrameHostImpl(
          base::BindRepeating(&RenderFrameHostImpl::BindNFCRequest)));
  // Creates a MojoRendererService, passing it a MediaPlayerRender.
  builder->AddInterface(CreateAlwaysAllowFilter(),
                        base::BindRepeating(&CreateMediaPlayerRenderer));
#else
  // The default (no-op) implementation of InstalledAppProvider. On Android, the
  // real implementation is provided in Java.
  builder->AddInterface(
      CreateContextTypeInterfaceFilter({WebContextType::kFrame}),
      base::BindRepeating([](blink::mojom::InstalledAppProviderRequest request,
                             RenderProcessHost*, const url::Origin&) {
        InstalledAppProviderImplDefault::Create(std::move(request));
      }));
  builder->AddInterface(CreateFeatureFilter(features::kWebAuth),
                        ForwardToRenderFrameHostImpl(base::BindRepeating(
                            &RenderFrameHostImpl::BindAuthenticatorRequest)));
#endif  // defined(OS_ANDROID)

#if BUILDFLAG(ENABLE_WEBRTC)
  builder->AddInterface(
      CreateAlwaysAllowFilter(),
      base::BindRepeating(
          [](blink::mojom::MediaDevicesDispatcherHostRequest request,
             RenderFrameHost* frame) {
            // BrowserMainLoop::GetInstance() may be null on unit tests.
            if (BrowserMainLoop::GetInstance()) {
              // BrowserMainLoop, which owns MediaStreamManager, is alive for
              // the lifetime of Mojo communication (see
              // BrowserMainLoop::ShutdownThreadsAndCleanUp(), which shuts down
              // Mojo). Hence, passing that MediaStreamManager instance as a raw
              // pointer here is safe.
              MediaStreamManager* media_stream_manager =
                  BrowserMainLoop::GetInstance()->media_stream_manager();
              BrowserThread::PostTask(
                  BrowserThread::IO, FROM_HERE,
                  base::BindOnce(&MediaDevicesDispatcherHost::Create,
                                 frame->GetProcess()->GetID(),
                                 frame->GetRoutingID(),
                                 base::Unretained(media_stream_manager),
                                 std::move(request)));
            }
          }));
#endif

#if BUILDFLAG(ENABLE_MEDIA_REMOTING)
  builder->AddInterface(CreateAlwaysAllowFilter(),
                        base::BindRepeating(&RemoterFactoryImpl::Bind));
#endif  // BUILDFLAG(ENABLE_MEDIA_REMOTING)

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
