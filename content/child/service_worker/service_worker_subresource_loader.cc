// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/service_worker/service_worker_subresource_loader.h"

#include "base/atomic_sequence_num.h"
#include "base/callback.h"
#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/child/service_worker/service_worker_event_dispatcher_holder.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/common/service_worker/service_worker_url_loader_helper.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "content/public/child/child_url_loader_factory_getter.h"
#include "content/public/common/content_features.h"
#include "net/base/io_buffer.h"
#include "net/http/http_util.h"
#include "third_party/WebKit/public/platform/Platform.h"
#include "third_party/WebKit/public/platform/WebBlobRegistry.h"
#include "ui/base/page_transition_types.h"

namespace content {

namespace {

int GetNextFetchEventID() {
  static base::AtomicSequenceNumber seq;
  return seq.GetNext();
}

}  // namespace

// ServiceWorkerSubresourceLoader -------------------------------------------

ServiceWorkerSubresourceLoader::ServiceWorkerSubresourceLoader(
    mojom::URLLoaderRequest request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& resource_request,
    mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
    scoped_refptr<ServiceWorkerEventDispatcherHolder> event_dispatcher,
    scoped_refptr<ChildURLLoaderFactoryGetter> default_loader_factory_getter,
    const GURL& controller_origin)
    : url_loader_client_(std::move(client)),
      url_loader_binding_(this, std::move(request)),
      response_callback_binding_(this),
      event_dispatcher_(std::move(event_dispatcher)),
      routing_id_(routing_id),
      request_id_(request_id),
      options_(options),
      resource_request_(resource_request),
      traffic_annotation_(traffic_annotation),
      controller_origin_(controller_origin),
      blob_client_binding_(this),
      default_loader_factory_getter_(std::move(default_loader_factory_getter)),
      weak_factory_(this) {
  DCHECK(event_dispatcher_ && event_dispatcher_->get());
  url_loader_binding_.set_connection_error_handler(base::BindOnce(
      &ServiceWorkerSubresourceLoader::DeleteSoon, weak_factory_.GetWeakPtr()));
  StartRequest(resource_request);
}

ServiceWorkerSubresourceLoader::~ServiceWorkerSubresourceLoader() {
  if (!blob_url_.is_empty()) {
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
  DCHECK_EQ(Status::kNotStarted, status_);
  status_ = Status::kStarted;

  mojom::ServiceWorkerFetchResponseCallbackPtr response_callback_ptr;
  response_callback_binding_.Bind(mojo::MakeRequest(&response_callback_ptr));

  event_dispatcher_->get()->DispatchFetchEvent(
      GetNextFetchEventID(), *request, mojom::FetchEventPreloadHandlePtr(),
      std::move(response_callback_ptr),
      base::Bind(&ServiceWorkerSubresourceLoader::OnFetchEventFinished,
                 weak_factory_.GetWeakPtr()));
}

std::unique_ptr<ServiceWorkerFetchRequest>
ServiceWorkerSubresourceLoader::CreateFetchRequest(
    const ResourceRequest& request) {
  return ServiceWorkerURLLoaderHelper::CreateFetchRequest(request);
}

void ServiceWorkerSubresourceLoader::OnFetchEventFinished(
    ServiceWorkerStatusCode status,
    base::Time dispatch_event_time) {
  if (status != SERVICE_WORKER_OK) {
    // TODO(kinuko): Log the failure.
    // If status is not OK response callback may not be called.
    weak_factory_.InvalidateWeakPtrs();
    OnFallback(dispatch_event_time);
  }
}

void ServiceWorkerSubresourceLoader::OnResponse(
    const ServiceWorkerResponse& response,
    base::Time dispatch_event_time) {
  StartResponse(response, nullptr /* body_as_blob */,
                nullptr /* body_as_stream */);
}

void ServiceWorkerSubresourceLoader::OnResponseBlob(
    const ServiceWorkerResponse& response,
    storage::mojom::BlobPtr body_as_blob,
    base::Time dispatch_event_time) {
  StartResponse(response, std::move(body_as_blob),
                nullptr /* body_as_stream */);
}

void ServiceWorkerSubresourceLoader::OnResponseStream(
    const ServiceWorkerResponse& response,
    blink::mojom::ServiceWorkerStreamHandlePtr body_as_stream,
    base::Time dispatch_event_time) {
  StartResponse(response, nullptr /* body_as_blob */,
                std::move(body_as_stream));
}

void ServiceWorkerSubresourceLoader::OnFallback(
    base::Time dispatch_event_time) {
  DCHECK(default_loader_factory_getter_);
  // Hand over to the network loader.
  // Per spec, redirects after this point are not intercepted by the service
  // worker again. (https://crbug.com/517364)
  default_loader_factory_getter_->GetNetworkLoaderFactory()
      ->CreateLoaderAndStart(url_loader_binding_.Unbind(), routing_id_,
                             request_id_, options_, resource_request_,
                             std::move(url_loader_client_),
                             traffic_annotation_);
  DeleteSoon();
}

void ServiceWorkerSubresourceLoader::StartResponse(
    const ServiceWorkerResponse& response,
    storage::mojom::BlobPtr body_asf_blob,
    blink::mojom::ServiceWorkerStreamHandlePtr body_as_stream) {
  // SaveResponseInfo and SaveResponseHeaders.
  ServiceWorkerURLLoaderHelper::MakeHeader(response, &response_head_);

  // Handle a stream response body.
  if (!body_as_stream.is_null() && body_as_stream->stream.is_valid()) {
    ServiceWorkerURLLoaderHelper::HandleStreamResponseBody(
        &url_loader_client_, &response_head_, ssl_info_, &body_as_stream);
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
    blink::WebString webstring_blob_uuid =
        blink::WebString::FromASCII(response.blob_uuid);
    blob_url_ =
        GURL("blob:" + controller_origin_.spec() + "/" + response.blob_uuid);
    blob_registry->RegisterPublicBlobURL(blob_url_, webstring_blob_uuid);
    // Decrement the Blob ref-count to offset AddRef in the controller Service
    // Worker code. TODO(kinuko): Remove this once RegisterPublicBlobURL can be
    // issued over mojo. Also see the comment around AddBlobDataRef in
    // ServiceWorkerContextClient for additional details. (crbug.com/756743)
    blob_registry->RemoveBlobDataRef(webstring_blob_uuid);
    VLOG(1) << "Reading blob: " << response.blob_uuid << " / "
            << blob_url_.spec();
    resource_request_.url = blob_url_;

    mojom::URLLoaderClientPtr blob_loader_client;
    blob_client_binding_.Bind(mojo::MakeRequest(&blob_loader_client));
    default_loader_factory_getter_->GetBlobLoaderFactory()
        ->CreateLoaderAndStart(mojo::MakeRequest(&blob_loader_), routing_id_,
                               request_id_, options_, resource_request_,
                               std::move(blob_loader_client),
                               traffic_annotation_);
    return;
  }

  // The response has no body.
  ServiceWorkerURLLoaderHelper::ResponseHasNoBody(
      &url_loader_client_, &response_head_, ssl_info_, net::OK);
}

void ServiceWorkerSubresourceLoader::SaveResponseInfo(
    const ServiceWorkerResponse& response) {
  ServiceWorkerURLLoaderHelper::SaveResponseInfo(response, &response_head_);
}

void ServiceWorkerSubresourceLoader::SaveResponseHeaders(
    int status_code,
    const std::string& status_text,
    const ServiceWorkerHeaderMap& headers) {
  ServiceWorkerURLLoaderHelper::SaveResponseHeaders(status_code, status_text,
                                                    headers, &response_head_);
}

void ServiceWorkerSubresourceLoader::CommitResponseHeaders() {
  DCHECK_EQ(Status::kStarted, status_);
  status_ = Status::kSentHeader;
  // TODO(kinuko): Fill the ssl_info.
  // Now there are  base::nullopt in ssl_info_.
  ServiceWorkerURLLoaderHelper::CommitResponseHeaders(
      &url_loader_client_, &response_head_, ssl_info_);
}

void ServiceWorkerSubresourceLoader::CommitCompleted(int error_code) {
  DCHECK_LT(status_, Status::kCompleted);
  status_ = Status::kCompleted;
  ServiceWorkerURLLoaderHelper::CommitCompleted(error_code,
                                                &url_loader_client_);
}

void ServiceWorkerSubresourceLoader::DeliverErrorResponse() {
  DCHECK_GT(status_, Status::kNotStarted);
  DCHECK_LT(status_, Status::kCompleted);
  if (status_ < Status::kSentHeader) {
    ServiceWorkerURLLoaderHelper::Send500(ssl_info_, &response_head_,
                                          &url_loader_client_);
    status_ = Status::kSentHeader;
  }
  ServiceWorkerURLLoaderHelper::CommitCompleted(net::ERR_FAILED,
                                                &url_loader_client_);
  status_ = Status::kCompleted;
}

// ServiceWorkerSubresourceLoader: URLLoader implementation -----------------

void ServiceWorkerSubresourceLoader::FollowRedirect() {
  // TODO(kinuko): Need to implement for the cases where a redirect is returned
  // by a ServiceWorker and the page determined to follow the redirect.
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
  ServiceWorkerURLLoaderHelper::OnReceiveResponse(
      response_head, ssl_info, std::move(downloaded_file), &response_head_,
      &url_loader_client_);
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
  // TODO(kinuko): Support this.
  NOTIMPLEMENTED();
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

ServiceWorkerSubresourceLoaderFactory::ServiceWorkerSubresourceLoaderFactory(
    scoped_refptr<ServiceWorkerEventDispatcherHolder> event_dispatcher,
    scoped_refptr<ChildURLLoaderFactoryGetter> default_loader_factory_getter,
    const GURL& controller_origin)
    : event_dispatcher_(std::move(event_dispatcher)),
      default_loader_factory_getter_(std::move(default_loader_factory_getter)),
      controller_origin_(controller_origin) {
  DCHECK_EQ(controller_origin, controller_origin.GetOrigin());
  DCHECK(default_loader_factory_getter_);
}

ServiceWorkerSubresourceLoaderFactory::
    ~ServiceWorkerSubresourceLoaderFactory() = default;

void ServiceWorkerSubresourceLoaderFactory::CreateLoaderAndStart(
    mojom::URLLoaderRequest request,
    int32_t routing_id,
    int32_t request_id,
    uint32_t options,
    const ResourceRequest& resource_request,
    mojom::URLLoaderClientPtr client,
    const net::MutableNetworkTrafficAnnotationTag& traffic_annotation) {
  // This loader destructs itself, as we want to transparently switch to the
  // network loader when fallback happens. When that happens the loader unbinds
  // the request, passes the request to the Network Loader Factory, and
  // destructs itself (while the loader client continues to work).
  new ServiceWorkerSubresourceLoader(
      std::move(request), routing_id, request_id, options, resource_request,
      std::move(client), traffic_annotation, event_dispatcher_,
      default_loader_factory_getter_, controller_origin_);
}

void ServiceWorkerSubresourceLoaderFactory::Clone(
    mojom::URLLoaderFactoryRequest request) {
  NOTREACHED();
}

}  // namespace content
