// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_SERVICE_WORKER_SERVICE_WORKER_LOADER_HELPERS_H_
#define CONTENT_COMMON_SERVICE_WORKER_SERVICE_WORKER_LOADER_HELPERS_H_

#include "content/common/service_worker/service_worker_types.h"
#include "content/public/common/url_loader_factory.mojom.h"
#include "net/ssl/ssl_info.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/service_worker_stream_handle.mojom.h"

namespace content {

// This class factor out common part between ServiceWorkerSubresourceLoader and
// ServiceWorkerURLLoaderJob.
class ServiceWorkerLoaderHelpers {
 public:
  static std::unique_ptr<ServiceWorkerFetchRequest> CreateFetchRequest(
      const ResourceRequest& request);

  static void PopulateResponseHead(const ServiceWorkerResponse& response,
                                   ResourceResponseHead* out_head);
  // Generates and populates |out_head->headers|.
  static void SaveResponseHeaders(const int status_code,
                                  const std::string& status_text,
                                  const ServiceWorkerHeaderMap& headers,
                                  ResourceResponseHead* out_head);
  // Populates |out_head| (except for headers) with given |response|.
  static void SaveResponseInfo(const ServiceWorkerResponse& response,
                               ResourceResponseHead* out_head);

  // Send Stream response to |url_loader_client| and load response body.
  static void SendStreamResponseToClient(
      const ResourceResponseHead* out_head,
      const base::Optional<net::SSLInfo>& ssl_info,
      mojo::ScopedDataPipeConsumerHandle& stream,
      mojom::URLLoaderClientPtr* url_loader_client);
  // Send blob response to |url_loader_client|.
  static void SendBlobResponseToClient(
      const ResourceResponseHead& response_head,
      const base::Optional<net::SSLInfo>& ssl_info,
      mojom::DownloadedTempFilePtr downloaded_file,
      ResourceResponseHead* out_head,
      mojom::URLLoaderClientPtr* url_loader_client);
  // Make header of status code 500 and send that response to
  // |url_loader_client|.
  static void SendErrorResponseToClient(
      const base::Optional<net::SSLInfo>& ssl_info,
      ResourceResponseHead* out_head,
      mojom::URLLoaderClientPtr* url_loader_client);
};

}  // namespace content

#endif  // CONTENT_COMMON_SERVICE_WORKER_SERVICE_WORKER_LOADER_HELPERS_H_
