// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/service_worker/service_worker_loader_helpers.h"

#include "base/strings/stringprintf.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "net/http/http_util.h"

namespace content {

// static
std::unique_ptr<ServiceWorkerFetchRequest>
ServiceWorkerLoaderHelpers::CreateFetchRequest(const ResourceRequest& request) {
  std::string blob_uuid;
  uint64_t blob_size = 0;
  // TODO(kinuko): Implement request.request_body handling.
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

// static
void ServiceWorkerLoaderHelpers::PopulateResponseHead(
    const ServiceWorkerResponse& response,
    ResourceResponseHead* out_head) {
  SaveResponseInfo(response, out_head);
  SaveResponseHeaders(response.status_code, response.status_text,
                      response.headers, out_head);
}

// static
void ServiceWorkerLoaderHelpers::SaveResponseHeaders(
    const int status_code,
    const std::string& status_text,
    const ServiceWorkerHeaderMap& headers,
    ResourceResponseHead* out_head) {
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

  out_head->headers = new net::HttpResponseHeaders(
      net::HttpUtil::AssembleRawHeaders(buf.c_str(), buf.size()));
  if (out_head->mime_type.empty()) {
    std::string mime_type;
    out_head->headers->GetMimeType(&mime_type);
    if (mime_type.empty())
      mime_type = "text/plain";
    out_head->mime_type = mime_type;
  }
}

// static
void ServiceWorkerLoaderHelpers::SaveResponseInfo(
    const ServiceWorkerResponse& response,
    ResourceResponseHead* out_head) {
  out_head->was_fetched_via_service_worker = true;
  out_head->was_fetched_via_foreign_fetch = false;
  out_head->was_fallback_required_by_service_worker = false;
  out_head->url_list_via_service_worker = response.url_list;
  out_head->response_type_via_service_worker = response.response_type;
  out_head->is_in_cache_storage = response.is_in_cache_storage;
  out_head->cache_storage_cache_name = response.cache_storage_cache_name;
  out_head->cors_exposed_header_names = response.cors_exposed_header_names;
  out_head->did_service_worker_navigation_preload = false;
}

// static
void ServiceWorkerLoaderHelpers::SendStreamResponseToClient(
    const ResourceResponseHead* out_head,
    const base::Optional<net::SSLInfo>& ssl_info,
    mojo::ScopedDataPipeConsumerHandle& stream,
    mojom::URLLoaderClientPtr* url_loader_client) {
  (*url_loader_client)
      ->OnReceiveResponse(*out_head, ssl_info, nullptr /* downloaded_file */);
  (*url_loader_client)->OnStartLoadingResponseBody(std::move(stream));
}

// static
void ServiceWorkerLoaderHelpers::SendBlobResponseToClient(
    const ResourceResponseHead& response_head,
    const base::Optional<net::SSLInfo>& ssl_info,
    mojom::DownloadedTempFilePtr downloaded_file,
    ResourceResponseHead* out_head,
    mojom::URLLoaderClientPtr* url_loader_client) {
  if (response_head.headers->response_code() >= 400) {
    DVLOG(1) << "Blob::SendBlobResponseToClient got error: "
             << response_head.headers->response_code();
    out_head->headers = response_head.headers;
  }
  (*url_loader_client)
      ->OnReceiveResponse(*out_head, ssl_info, std::move(downloaded_file));
}

// static
void ServiceWorkerLoaderHelpers::SendErrorResponseToClient(
    const base::Optional<net::SSLInfo>& ssl_info,
    ResourceResponseHead* out_head,
    mojom::URLLoaderClientPtr* url_loader_client) {
  SaveResponseHeaders(500, "Service Worker Response Error",
                      ServiceWorkerHeaderMap(), out_head);
  (*url_loader_client)
      ->OnReceiveResponse(*out_head, ssl_info, nullptr /* downloaded_file */);
}

}  // namespace content
