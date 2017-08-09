// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/service_worker/service_worker_subresource_loader.h"

#include "base/callback.h"
#include "content/common/service_worker/service_worker_event_dispatcher.mojom.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "content/common/service_worker/service_worker_types.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "ui/base/page_transition_types.h"

namespace content {

// ServiceWorkerSubresourceLoader::ResponseCallback ---------------

// Helper to receive the fetch event response even if
// ServiceWorkerFetchDispatcher has been destroyed.
class ServiceWorkerSubresourceLoader::ResponseCallback
    : public mojom::ServiceWorkerFetchResponseCallback {
 public:
  explicit ResponseCallback(
      base::WeakPtr<ServiceWorkerSubresourceLoader> loader)
      : loader_(loader),
        binding_(this) {}
  ~ResponseCallback() override {}

  void Run(int request_id,
           const ServiceWorkerResponse& response,
           base::Time dispatch_event_time) {
    DCHECK(response.blob_uuid.size());
    HandleResponse(response, blink::mojom::ServiceWorkerStreamHandlePtr(),
                   SERVICE_WORKER_FETCH_EVENT_RESULT_RESPONSE,
                   dispatch_event_time);
  }

  // Implements mojom::ServiceWorkerFetchResponseCallback.
  void OnResponse(const ServiceWorkerResponse& response,
                  base::Time dispatch_event_time) override {
    HandleResponse(response, blink::mojom::ServiceWorkerStreamHandlePtr(),
                   SERVICE_WORKER_FETCH_EVENT_RESULT_RESPONSE,
                   dispatch_event_time);
  }
  void OnResponseStream(
      const ServiceWorkerResponse& response,
      blink::mojom::ServiceWorkerStreamHandlePtr body_as_stream,
      base::Time dispatch_event_time) override {
    HandleResponse(response, std::move(body_as_stream),
                   SERVICE_WORKER_FETCH_EVENT_RESULT_RESPONSE,
                   dispatch_event_time);
  }
  void OnFallback(base::Time dispatch_event_time) override {
    HandleResponse(
        ServiceWorkerResponse(), blink::mojom::ServiceWorkerStreamHandlePtr(),
        SERVICE_WORKER_FETCH_EVENT_RESULT_FALLBACK, dispatch_event_time);
  }

  mojom::ServiceWorkerFetchResponseCallbackPtr CreateInterfacePtrAndBind() {
    mojom::ServiceWorkerFetchResponseCallbackPtr callback_proxy;
    binding_.Bind(mojo::MakeRequest(&callback_proxy));
    return callback_proxy;
  }

 private:
  void HandleResponse(const ServiceWorkerResponse& response,
                      blink::mojom::ServiceWorkerStreamHandlePtr body_as_stream,
                      ServiceWorkerFetchEventResult fetch_result,
                      base::Time dispatch_event_time) {
    if (!loader_)
      return;
    loader_->StartResponse(fetch_result, response,
                           std::move(body_as_stream));
  }

  base::WeakPtr<ServiceWorkerSubresourceLoader> loader_;
  mojo::StrongBinding<mojom::ServiceWorkerFetchResponseCallback> binding_;

  DISALLOW_COPY_AND_ASSIGN(ResponseCallback);
};

// ServiceWorkerSubresourceLoader ---------------------------------

ServiceWorkerSubresourceLoader::ServiceWorkerSubresourceLoader(
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& resource_request,
    mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    mojom::ServiceWorkerEventDispatcher* event_dispatcher,
    mojom::URLLoaderFactory* blob_loader_factory)
    : client_(std::move(client)),
      event_dispatcher_(event_dispatcher),
      blob_loader_factory_(blob_loader_factory),
      weak_factory_(this) {
  DCHECK(event_dispatcher_);
  StartRequest(resource_request);
}

ServiceWorkerSubresourceLoader::~ServiceWorkerSubresourceLoader() {}

void ServiceWorkerSubresourceLoader::StartRequest(
    const ResourceRequest& resource_request) {
  auto request = CreateFetchRequest(resource_request);

  DCHECK(!response_callback_);
  response_callback_ = base::MakeUnique<ResponseCallback>(
      weak_factory_.GetWeakPtr());
  mojom::ServiceWorkerFetchResponseCallbackPtr response_callback_ptr =
      response_callback_->CreateInterfacePtrAndBind();

  event_dispatcher_->DispatchFetchEvent(
      0 /* fetch_event_id (unused) */, *request,
      mojom::FetchEventPreloadHandlePtr(),
      std::move(response_callback_ptr),
      base::Bind(&ServiceWorkerSubresourceLoader::OnFetchEventFinished,
                 weak_factory_.GetWeakPtr()));
}

std::unique_ptr<ServiceWorkerFetchRequest>
ServiceWorkerSubresourceLoader::CreateFetchRequest(
    const ResourceRequest& request) {
  std::string blob_uuid;
  uint64_t blob_size = 0;
  // TODO(kinuko): Implement request_body handling.
  DCHECK(!request.request_body);
  auto new_request = base::MakeUnique<ServiceWorkerFetchRequest>();
  new_request->mode = request.fetch_request_mode;
  new_request->is_main_resource_load =
      ServiceWorkerUtils::IsMainResourceType(request.resource_type);
  new_request->request_context_type = request.fetch_request_context_type;
  new_request->frame_type = request.fetch_frame_type;
  new_request->url = request.url;
  new_request->method = request.method;
  new_request->blob_uuid = blob_uuid;
  new_request->blob_size = blob_size;
  new_request->credentials_mode = request.fetch_credentials_mode;
  new_request->redirect_mode = request.fetch_redirect_mode;
  new_request->is_reload = ui::PageTransitionCoreTypeIs(
      request.transition_type, ui::PAGE_TRANSITION_RELOAD);
  new_request->referrer =
      Referrer(GURL(request.referrer), request.referrer_policy);
  new_request->fetch_type = ServiceWorkerFetchType::FETCH;
  return new_request;
}

void ServiceWorkerSubresourceLoader::OnFetchEventFinished(
    ServiceWorkerStatusCode status,
    base::Time dispatch_event_time) {
  // Placeholder for now.
}

void ServiceWorkerSubresourceLoader::StartResponse(
    ServiceWorkerFetchEventResult fetch_result,
    const ServiceWorkerResponse& response,
    blink::mojom::ServiceWorkerStreamHandlePtr body_as_stream) {
}

void ServiceWorkerSubresourceLoader::FollowRedirect() {
  // TODO(kinuko): We need to re-send the redirected request to the
  // ServiceWorker.
  NOTIMPLEMENTED();
}

void ServiceWorkerSubresourceLoader::SetPriority(
    net::RequestPriority priority, int intra_priority_value) {
  // Not supported (do nothing).
}

void ServiceWorkerSubresourceLoaderFactory::CreateAndBind(
    mojom::URLLoaderFactoryRequest request,
    mojom::ServiceWorkerEventDispatcher* event_dispatcher,
    URLLoaderFactory* blob_loader_factory) {
  mojo::MakeStrongBinding(
      base::MakeUnique<ServiceWorkerSubresourceLoaderFactory>(
          event_dispatcher, blob_loader_factory),
      std::move(request));
}

// ServiceWorkerSubresourceLoaderFactory --------------------------

ServiceWorkerSubresourceLoaderFactory::ServiceWorkerSubresourceLoaderFactory(
    mojom::ServiceWorkerEventDispatcher* event_dispatcher,
    mojom::URLLoaderFactory* blob_loader_factory)
    : event_dispatcher_(event_dispatcher),
      blob_loader_factory_(blob_loader_factory) {}

ServiceWorkerSubresourceLoaderFactory::
~ServiceWorkerSubresourceLoaderFactory() {}

void ServiceWorkerSubresourceLoaderFactory::CreateLoaderAndStart(
    mojom::URLLoaderRequest request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& resource_request,
    mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  mojo::MakeStrongBinding(
      base::MakeUnique<ServiceWorkerSubresourceLoader>(
          routing_id, request_id, options, resource_request, std::move(client),
          traffic_annotation, event_dispatcher_, blob_loader_factory_),
      std::move(request));
}

void ServiceWorkerSubresourceLoaderFactory::SyncLoad(
    int32_t routing_id,
    int32_t request_id,
    const ResourceRequest& resource_request,
    SyncLoadCallback callback) {
  NOTREACHED();
}

}  // namespace content
