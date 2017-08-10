// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/service_worker/service_worker_subresource_loader.h"

#include "base/callback.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/common/service_worker/service_worker_event_dispatcher.mojom.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/base/io_buffer.h"
#include "net/http/http_util.h"
#include "third_party/WebKit/public/platform/Platform.h"
#include "third_party/WebKit/public/platform/WebBlobRegistry.h"
#include "ui/base/page_transition_types.h"

namespace content {

// ServiceWorkerSubresourceLoader::ResponseCallback -------------------------

// Helper to receive the fetch event response even if
// ServiceWorkerFetchDispatcher has been destroyed.
class ServiceWorkerSubresourceLoader::ResponseCallback
    : public mojom::ServiceWorkerFetchResponseCallback {
 public:
  explicit ResponseCallback(
      base::WeakPtr<ServiceWorkerSubresourceLoader> loader)
      : loader_(loader), binding_(this) {}
  ~ResponseCallback() override {}

  void Run(int request_id,
           const ServiceWorkerResponse& response,
           base::Time dispatch_event_time) {
    DCHECK(response.blob_uuid.size());
    if (loader_) {
      loader_->StartResponse(response,
                             blink::mojom::ServiceWorkerStreamHandlePtr());
    }
    loader_.reset();
  }

  // Implements mojom::ServiceWorkerFetchResponseCallback.
  void OnResponse(const ServiceWorkerResponse& response,
                  base::Time dispatch_event_time) override {
    if (loader_) {
      loader_->StartResponse(response,
                             blink::mojom::ServiceWorkerStreamHandlePtr());
    }
    loader_.reset();
  }
  void OnResponseStream(
      const ServiceWorkerResponse& response,
      blink::mojom::ServiceWorkerStreamHandlePtr body_as_stream,
      base::Time dispatch_event_time) override {
    if (loader_)
      loader_->StartStreamResponse(response, std::move(body_as_stream));
    loader_.reset();
  }
  void OnFallback(base::Time dispatch_event_time) override {
    if (loader_)
      loader_->FallbackToNetwork();
    loader_.reset();
  }

  mojom::ServiceWorkerFetchResponseCallbackPtr CreateInterfacePtrAndBind() {
    mojom::ServiceWorkerFetchResponseCallbackPtr callback_proxy;
    binding_.Bind(mojo::MakeRequest(&callback_proxy));
    return callback_proxy;
  }

 private:
  base::WeakPtr<ServiceWorkerSubresourceLoader> loader_;
  mojo::StrongBinding<mojom::ServiceWorkerFetchResponseCallback> binding_;

  DISALLOW_COPY_AND_ASSIGN(ResponseCallback);
};

// ServiceWorkerSubresourceLoader -------------------------------------------

ServiceWorkerSubresourceLoader::ServiceWorkerSubresourceLoader(
    mojom::URLLoaderRequest request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& resource_request,
    mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    mojom::ServiceWorkerEventDispatcher* event_dispatcher,
    base::WeakPtr<URLLoaderFactoryContainer> loader_factory_container,
    const GURL& controller_origin)
    : url_loader_client_(std::move(client)),
      url_loader_binding_(this, std::move(request)),
      event_dispatcher_(event_dispatcher),
      routing_id_(routing_id),
      request_id_(request_id),
      options_(options),
      resource_request_(resource_request),
      traffic_annotation_(traffic_annotation),
      loader_factory_container_(loader_factory_container),
      blob_client_binding_(this),
      controller_origin_(controller_origin),
      weak_factory_(this) {
  DCHECK(event_dispatcher_);
  binding_.set_connection_error_handler(base::BindOnce(
      &ServiceWorkerSubresourceLoader::DeleteSoon, weak_factory_.GetWeakPtr()));
  StartRequest(resource_request);
}

ServiceWorkerSubresourceLoader::~ServiceWorkerSubresourceLoader() {
  if (!blob_url_.empty()) {
    blink::Platform::Current()->GetBlobRegistry()->RevokePublicBlobURL(
        blob_url_);
  }
}

void ServiceWorkerSubresourceLoader::DeleteSoon() {
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

void ServiceWorkerSubresourceLoader::StartRequest(
    const ResourceRequest& resource_request) {
  auto request = CreateFetchRequest(resource_request);

  DCHECK(!response_callback_);
  response_callback_ =
      base::MakeUnique<ResponseCallback>(weak_factory_.GetWeakPtr());
  mojom::ServiceWorkerFetchResponseCallbackPtr response_callback_ptr =
      response_callback_->CreateInterfacePtrAndBind();

  event_dispatcher_->DispatchFetchEvent(
      0 /* fetch_event_id (unused) */, *request,
      mojom::FetchEventPreloadHandlePtr(), std::move(response_callback_ptr),
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
  // Just a placeholder for now.
}

void ServiceWorkerSubresourceLoader::FallbackToNetwork() {
  DCHECK(loader_factory_container_);
  // Hand over to the network loader. Redirect after this wouldn't need
  // to be intercepted by SW.
  loader_factory_container_->network_loader_factory()->CreateLoaderAndStart(
      url_loader_binding_.Unbind(), routing_id_, request_id_, options_,
      resource_request_, std::move(url_loader_client_), traffic_annotation_);
  DeleteSoon();
}

void ServiceWorkerSubresourceLoader::StartResponse(
    const ServiceWorkerResponse& response,
    blink::mojom::ServiceWorkerStreamHandlePtr body_as_stream) {
  SaveResponseInfo(response);
  SaveResponseHeaders(response.status_code, response.status_text,
                      response.headers);

  // Handle a stream response body.
  if (!body_as_stream.is_null() && body_as_stream->stream.is_valid()) {
    CommitResponseHeaders();
    url_loader_client_->OnStartLoadingResponseBody(
        std::move(body_as_stream->stream));
    CommitCompleted(net::OK);
    return;
  }

  // Handle a blob response body. Ideally we'd just get a data pipe from
  // SWFetchDispatcher, and this could be treated the same as a stream response.
  // See:
  // https://docs.google.com/a/google.com/document/d/1_ROmusFvd8ATwIZa29-P6Ls5yyLjfld0KvKchVfA84Y/edit?usp=drive_web
  // TODO(kinuko): This code is hacked up on top of the legacy API, migrate
  // to mojo-fied Blob code once it becomes ready.
  blink::WebBlobRegistry* blob_registry =
      blink::Platform::Current()->GetBlobRegistry();
  if (!response.blob_uuid.empty() && blob_registry) {
    DCHECK(loader_factory_container_);
    blob_url_ =
        GURL("blob:" + controller_origin_.spec() + "/" + response.blob_uuid);
    blob_registry->RegisterPublicBlobURL(blob_url_, response.blob_uuid);
    resource_request_.url = blob_url_;

    mojom::URLLoaderRequest request;
    mojom::URLLoaderClientPtr client;
    blob_client_binding_.Bind(mojo::MakeRequest(&client));
    loader_factory_container_->blob_loader_factory()->CreateLoaderAndStart(
        std::move(request), routing_id_, request_id_, options_,
        resource_request_, std::move(client), traffic_annotation_);
    return;
  }

  // The response has no body.
  CommitResponseHeaders();
  CommitCompleted(net::OK);
}

void ServiceWorkerSubresourceLoader::SaveResponseInfo(
    const ServiceWorkerResponse& response) {
  response_head_.was_fetched_via_service_worker = true;
  response_head_.was_fetched_via_foreign_fetch = false;
  response_head_.was_fallback_required_by_service_worker = false;
  response_head_.url_list_via_service_worker = response.url_list;
  response_head_.response_type_via_service_worker = response.response_type;
  response_head_.is_in_cache_storage = response.is_in_cache_storage;
  response_head_.cache_storage_cache_name = response.cache_storage_cache_name;
  response_head_.cors_exposed_header_names = response.cors_exposed_header_names;
  response_head_.did_service_worker_navigation_preload =
      did_navigation_preload_;
}

void ServiceWorkerSubresourceLoader::SaveResponseHeaders(
    int status_code,
    const std::string& status_text,
    const ServiceWorkerHeaderMap& headers) {
  // Build a string instead of using HttpResponseHeaders::AddHeader on
  // each header, since AddHeader has O(n^2) performance.
  std::string buf(base::StringPrintf("HTTP/1.1 %d %s\r\n", status_code,
                                     status_text.c_str()));
  for (const auto& item : headers) {
    buf.append(item.first);
    buf.append(": ");
    buf.append(item.second);
    buf.append("\r\n");
  }
  buf.append("\r\n");

  response_head_.headers = new net::HttpResponseHeaders(
      net::HttpUtil::AssembleRawHeaders(buf.c_str(), buf.size()));
  if (response_head_.mime_type.empty()) {
    std::string mime_type;
    response_head_.headers->GetMimeType(&mime_type);
    if (mime_type.empty())
      mime_type = "text/plain";
    response_head_.mime_type = mime_type;
  }
}

void ServiceWorkerSubresourceLoader::CommitResponseHeaders() {
  DCHECK_EQ(Status::kStarted, status_);
  status_ = Status::kSentHeader;
  // TODO(kinuko): Fill the ssl_info.
  url_loader_client_->OnReceiveResponse(response_head_,
                                        base::nullopt /* ssl_info */, nullptr);
}

void ServiceWorkerSubresourceLoader::CommitCompleted(int error_code) {
  DCHECK_LT(status_, Status::kCompleted);
  status_ = Status::kCompleted;
  ResourceRequestCompletionStatus completion_status;
  completion_status.error_code = error_code;
  completion_status.completion_time = base::TimeTicks::Now();
  url_loader_client_->OnComplete(completion_status);
}

void ServiceWorkerURLLoaderJob::DeliverErrorResponse() {
  DCHECK_GT(status_, Status::kNotStarted);
  DCHECK_LT(status_, Status::kCompleted);
  if (status_ < Status::kSentHeader) {
    SaveResponseHeaders(500, "Service Worker Response Error",
                        ServiceWorkerHeaderMap());
    CommitResponseHeaders();
  }
  CommitCompleted(net::ERR_FAILED);
}

// ServiceWorkerSubresourceLoader: URLLoader implementation -----------------

void ServiceWorkerSubresourceLoader::FollowRedirect() {
  // TODO(kinuko): We need to re-send the redirected request to the
  // ServiceWorker.
  NOTIMPLEMENTED();
}

void ServiceWorkerSubresourceLoader::SetPriority(net::RequestPriority priority,
                                                 int intra_priority_value) {
  // Not supported (do nothing).
}

// ServiceWorkerSubresourceLoader: URLLoaderClient for Blob reading ---------

void ServiceWorkerSubresourceLoader::OnReceiveResponse(
    const ResourceResponseHead& response_head,
    const base::Optional<net::SSLInfo>& ssl_info,
    mojom::DownloadedTempFilePtr downloaded_file) {
  DCHECK_EQ(Status::kStarted, status_);
  status_ = Status::kSentHeader;
  if (response_head.headers->response_code() >= 400)
    response_head_.headers = response_head.headers;
  url_loader_client_->OnReceiveResponse(response_head_, ssl_info,
                                        std::move(downloaded_file));
}

void ServiceWorkerSubresourceLoader::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const ResourceResponseHead& response_head) {
  NOTREACHED();
}

void ServiceWorkerSubresourceLoader::OnDataDownloaded(
    int64_t data_len,
    int64_t encoded_data_len) {
  NOTREACHED();
}

void ServiceWorkerSubresourceLoader::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback ack_callback) {
  NOTREACHED();
}

void ServiceWorkerSubresourceLoader::OnReceiveCachedMetadata(
    const std::vector<uint8_t>& data) {
  NOTREACHED();
}

void ServiceWorkerSubresourceLoader::OnTransferSizeUpdated(
    int32_t transfer_size_diff) {
  NOTREACHED();
}

void ServiceWorkerSubresourceLoader::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  url_loader_client_->OnStartLoadingResponseBody(std::move(body));
}

void ServiceWorkerSubresourceLoader::OnComplete(
    const ResourceRequestCompletionStatus& status) {
  DCHECK_EQ(Status::kSentHeader, status_);
  status_ = Status::kCompleted;
  url_loader_client_->OnComplete(status);
}

// ServiceWorkerSubresourceLoaderFactory ------------------------------------

void ServiceWorkerSubresourceLoaderFactory::CreateAndBind(
    mojom::URLLoaderFactoryRequest request,
    mojom::ServiceWorkerEventDispatcher* event_dispatcher,
    base::WeakPtr<URLLoaderFactoryContainer> loader_factory_container,
    const GURL& controller_origin) {
  mojo::MakeStrongBinding(
      base::MakeUnique<ServiceWorkerSubresourceLoaderFactory>(
          event_dispatcher, loader_factory_container),
      std::move(request));
}

ServiceWorkerSubresourceLoaderFactory::ServiceWorkerSubresourceLoaderFactory(
    mojom::ServiceWorkerEventDispatcher* event_dispatcher,
    base::WeakPtr<URLLoaderFactoryContainer> loader_factory_container,
    const GURL& controller_origin)
    : event_dispatcher_(event_dispatcher),
      loader_factory_container_(loader_factory_container),
      controller_origin_(controller_origin) {
  DCHECK_EQ(controller_origin, controller_origin.GetOrigin());
  DCHECK(loader_factory_container_);
}

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
  // This should be non-null as far as the frame is around.
  DCHECK(loader_factory_container_);
  // This loader destructs itself.
  new ServiceWorkerSubresourceLoader(
      std::move(request), routing_id, request_id, options, resource_request,
      std::move(client), traffic_annotation, event_dispatcher_,
      loader_factory_container_, controller_origin_);
}

void ServiceWorkerSubresourceLoaderFactory::SyncLoad(
    int32_t routing_id,
    int32_t request_id,
    const ResourceRequest& resource_request,
    SyncLoadCallback callback) {
  NOTREACHED();
}

}  // namespace content
