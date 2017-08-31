// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_SERVICE_WORKER_SERVICE_WORKER_URL_LOADER_HELPER_H_
#define CONTENT_COMMON_SERVICE_WORKER_SERVICE_WORKER_URL_LOADER_HELPER_H_

#include "content/common/service_worker/service_worker_types.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/service_worker_stream_handle.mojom.h"

namespace content {

// This class factor out common part between ServiceWorkerSubresourceLoader and
// ServiceWorkerURLLoaderJob.
class ServiceWorkerURLLoaderHelper {
 public:
  // Expected to be called after CommitResponseHeaders (i.e. status_ ==
  // kSentHeader). Expected to set |status_|  Status::kCompleted after call
  // this. Methods that use this: CommitCompleted, DeliverErrorResponse.
  static void CommitCompleted(const int error_code,
                              mojom::URLLoaderClientPtr* url_loader_client);

  // Expected to be called after saving response info/headers.
  // Methods that use this: CommitResponseHeaders.
  static void CommitResponseHeaders(
      mojom::URLLoaderClientPtr* url_loader_client,
      const ResourceResponseHead* out_head,
      const base::Optional<net::SSLInfo>& ssl_info);

  // Methods that use this: CreateFetchRequest.
  static std::unique_ptr<ServiceWorkerFetchRequest> CreateFetchRequest(
      const ResourceRequest& request);

  // mojom::URLLoaderClient for Blob response reading (used only when
  // the service worker response had valid Blob UUID).
  // Methods that use this: OnReceiveResponse.
  static void OnReceiveResponse(const ResourceResponseHead& response_head,
                                const base::Optional<net::SSLInfo>& ssl_info,
                                mojom::DownloadedTempFilePtr downloaded_file,
                                ResourceResponseHead* out_head,
                                mojom::URLLoaderClientPtr* url_loader_client);

  // Generates and populates |out_head->headers|.
  // Methods that use this: SaveResponseHeaders.
  static void SaveResponseHeaders(const int status_code,
                                  const std::string& status_text,
                                  const ServiceWorkerHeaderMap& headers,
                                  ResourceResponseHead* out_head);

  // Populates |out_head| (except for headers) with given |response|.
  // Methods that use this: SaveResponseInfo.
  static void SaveResponseInfo(const ServiceWorkerResponse& response,
                               ResourceResponseHead* out_head);

  // Make header of status code 500. Expected to set |status_|
  // Status::kSentHeader after call this. Methods that use this:
  // Methods that use this: DeliverErrorResponse.
  static void Send500(const base::Optional<net::SSLInfo>& ssl_info,
                      ResourceResponseHead* out_head,
                      mojom::URLLoaderClientPtr* url_loader_client);

  // Methods that use this: StartResponse
  static void MakeHeader(const ServiceWorkerResponse& response,
                         ResourceResponseHead* out_head);
  static void HandleStreamResponseBody(
      mojom::URLLoaderClientPtr* url_loader_client,
      const ResourceResponseHead* out_head,
      const base::Optional<net::SSLInfo>& ssl_info,
      blink::mojom::ServiceWorkerStreamHandlePtr* body_as_stream);
  static void ResponseHasNoBody(mojom::URLLoaderClientPtr* url_loader_client,
                                const ResourceResponseHead* out_head,
                                const base::Optional<net::SSLInfo>& ssl_info,
                                const int error_code);
};

}  // namespace content

#endif  // CONTENT_COMMON_SERVICE_WORKER_SERVICE_WORKER_URL_LOADER_HELPER_H_
