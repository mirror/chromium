// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_SERVICE_WORKER_SERVICE_WORKER_SUBRESOURCE_LOADER_H_
#define CONTENT_CHILD_SERVICE_WORKER_SERVICE_WORKER_SUBRESOURCE_LOADER_H_

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "content/common/content_export.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/traffic_annotation/network_traffic_annotation.h"

namespace content {

namespace mojom {
class ServiceWorkerEventDispatcher;
}

class ServiceWorkerFetchRequest;
class URLLoaderContainerFactory;

// TODO(kinuko): Consider factoring out the common part with
// ServiceWorkerURLLoaderJob into WebKit/common.
class CONTENT_EXPORT ServiceWorkerSubresourceLoader
    : public mojom::URLLoader,
      public mojom::URLLoaderClient {
 public:
  // |event_dispatcher|, |network_loader_factory|  and |blob_loader_factory|
  // are supposed to outlive this instance.
  ServiceWorkerSubresourceLoader(
      mojom::URLLoaderRequest request,
      int32_t routing_id,
      int32_t request_id,
      uint32_t options,
      const ResourceRequest& resource_request,
      mojom::URLLoaderClientPtr client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation,
      mojom::ServiceWorkerEventDispatcher* event_dispatcher,
      base::WeakPtr<URLLoaderFactoryContainer> loader_factory_container,
      const GURL& controller_origin);

  ~ServiceWorkerSubresourceLoader() override;

 private:
  class ResponseCallback;

  void DeleteSoon();

  void StartRequest(const ResourceRequest& resource_request);
  std::unique_ptr<ServiceWorkerFetchRequest> CreateFetchRequest(
      const ResourceRequest& request);
  void OnFetchEventFinished(ServiceWorkerStatusCode status,
                            base::Time dispatch_event_time);
  void FallbackToNetwork();
  void StartResponse(const ServiceWorkerResponse& response,
                     blink::mojom::ServiceWorkerStreamHandlePtr body_as_stream);
  void AfterRead(scoped_refptr<net::IOBuffer> buffer, int bytes);

  // mojom::URLLoader overrides:
  void FollowRedirect() override;
  void SetPriority(net::RequestPriority priority,
                   int intra_priority_value) override;

  // Populates |response_info_| (except for headers) with given |response|.
  void SaveResponseInfo(const ServiceWorkerResponse& response);
  // Generates and populates |response.headers|.
  void SaveResponseHeaders(int status_code,
                           const std::string& status_text,
                           const ServiceWorkerHeaderMap& headers);
  // Calls url_loader_client_->OnReceiveResopnse() with |response_head_|.
  // Expected to be called afer saving response info/headers.
  void CommitResponseHeaders();
  // Calls url_loader_client_->OnComplete(). Expected to be called after
  // CommitResponseHeaders (i.e. status_ == kSentHeader).
  void CommitCompleted(int error_code);
  // Calls CommitResponseHeaders() if we haven't sent headers yet,
  // and CommitCompleted() with error code.
  void DeliverErrorResponse();

  // mojom::URLLoaderClient for Blob response reading (used only when
  // the SW response had valid blob UUID):
  void OnReceiveResponse(const ResourceResponseHead& response_head,
                         const base::Optional<net::SSLInfo>& ssl_info,
                         mojom::DownloadedTempFilePtr downloaded_file) override;
  void OnReceiveRedirect(const net::RedirectInfo& redirect_info,
                         const ResourceResponseHead& response_head) override;
  void OnDataDownloaded(int64_t data_len, int64_t encoded_data_len) override;
  void OnUploadProgress(int64_t current_position,
                        int64_t total_size,
                        OnUploadProgressCallback ack_callback) override;
  void OnReceiveCachedMetadata(const std::vector<uint8_t>& data) override;
  void OnTransferSizeUpdated(int32_t transfer_size_diff) override;
  void OnStartLoadingResponseBody(
      mojo::ScopedDataPipeConsumerHandle body) override;
  void OnComplete(const ResourceRequestCompletionStatus& status) override;

  std::unique_ptr<ResponseCallback> response_callback_;
  ResourceResponseHead response_head_;

  mojom::URLLoaderClientPtr url_loader_client_;
  mojo::Binding<mojom::URLLoader> url_loader_binding_;

  mojom::ServiceWorkerEventDispatcher* event_dispatcher_;

  const int routing_id_;
  const int request_id_;
  uint32_t options_;
  ResourceRequest resource_request_;
  net::MutableNetworkTrafficAnnotationTag traffic_annotation_,

      // To load a blob.
      GURL blob_url_;
  GURL controller_origin_;
  base::WeakPtr<URLLoaderFactoryContainer> loader_factory_container_;
  mojo::Binding<mojom::URLLoaderClient> blob_client_binding_;

  // For fallback.
  mojom::URLLoaderFactory* network_loader_factory_;

  enum class Status {
    kNotStarted,
    kStarted,
    kSentHeader,
    kCompleted,
    kCancelled
  };
  Status status_ = Status::kNotStarted;

  base::WeakPtrFactory<ServiceWorkerSubresourceLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerSubresourceLoader);
};

class CONTENT_EXPORT ServiceWorkerSubresourceLoaderFactory
    : public mojom::URLLoaderFactory {
 public:
  static void CreateAndBind(
      mojom::URLLoaderFactoryRequest request,
      mojom::ServiceWorkerEventDispatcher* event_dispatcher,
      base::WeakPtr<URLLoaderFactoryContainer> loader_factory_container,
      const GURL& controller_origin);

  ServiceWorkerSubresourceLoaderFactory(
      mojom::ServiceWorkerEventDispatcher* event_dispatcher,
      base::WeakPtr<URLLoaderFactoryContainer> loader_factory_container,
      const GURL& controller_origin);

  ~ServiceWorkerSubresourceLoaderFactory() override;

  void CreateLoaderAndStart(mojom::URLLoaderRequest request,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const ResourceRequest& resource_request,
                            mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;

  void SyncLoad(int32_t routing_id,
                int32_t request_id,
                const ResourceRequest& resource_request,
                SyncLoadCallback callback) override;

 private:
  mojom::ServiceWorkerEventDispatcher* event_dispatcher_;
  base::WeakPtr<URLLoaderFactoryContainer> loader_factory_container,
      GURL controller_origin_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerSubresourceLoaderFactory);
};

}  // namespace content

#endif  // CONTENT_CHILD_SERVICE_WORKER_SERVICE_WORKER_SUBRESOURCE_LOADER_H_
