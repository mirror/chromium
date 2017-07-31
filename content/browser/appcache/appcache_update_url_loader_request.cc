// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/appcache_update_url_loader_request.h"
#include "content/browser/appcache/appcache_update_url_fetcher.h"
#include "net/url_request/url_request_context.h"

namespace content {

AppCacheUpdateURLLoaderRequest::~AppCacheUpdateURLLoaderRequest() {}

void AppCacheUpdateURLLoaderRequest::Start() {
  NOTIMPLEMENTED();
}

void AppCacheUpdateURLLoaderRequest::SetExtraRequestHeaders(
    const net::HttpRequestHeaders& headers) {
  NOTIMPLEMENTED();
}

GURL AppCacheUpdateURLLoaderRequest::GetURL() const {
  NOTIMPLEMENTED();
  return GURL();
}

GURL AppCacheUpdateURLLoaderRequest::GetOriginalURL() const {
  NOTIMPLEMENTED();
  return GURL();
}

void AppCacheUpdateURLLoaderRequest::SetLoadFlags(int flags) {
  NOTIMPLEMENTED();
}

int AppCacheUpdateURLLoaderRequest::GetLoadFlags() const {
  NOTIMPLEMENTED();
  return 0;
}

std::string AppCacheUpdateURLLoaderRequest::GetMimeType() const {
  NOTIMPLEMENTED();
  return std::string();
}

void AppCacheUpdateURLLoaderRequest::SetFirstPartyForCookies(
    const GURL& first_party_for_cookies) {
  NOTIMPLEMENTED();
}

void AppCacheUpdateURLLoaderRequest::SetInitiator(
    const base::Optional<url::Origin>& initiator) {
  NOTIMPLEMENTED();
}

net::HttpResponseHeaders* AppCacheUpdateURLLoaderRequest::GetResponseHeaders()
    const {
  NOTIMPLEMENTED();
  return nullptr;
}

int AppCacheUpdateURLLoaderRequest::GetResponseCode() const {
  NOTIMPLEMENTED();
  return 0;
}

net::HttpResponseInfo AppCacheUpdateURLLoaderRequest::GetResponseInfo() const {
  NOTIMPLEMENTED();
  return net::HttpResponseInfo();
}

const net::URLRequestContext*
AppCacheUpdateURLLoaderRequest::GetRequestContext() const {
  NOTIMPLEMENTED();
  return nullptr;
}

int AppCacheUpdateURLLoaderRequest::Read(net::IOBuffer* buf, int max_bytes) {
  NOTIMPLEMENTED();
  return 0;
}

int AppCacheUpdateURLLoaderRequest::Cancel() {
  NOTIMPLEMENTED();
  return 0;
}

void AppCacheUpdateURLLoaderRequest::OnReceiveResponse(
    const ResourceResponseHead& response_head,
    const base::Optional<net::SSLInfo>& ssl_info,
    mojom::DownloadedTempFilePtr downloaded_file) {
  NOTIMPLEMENTED();
}

void AppCacheUpdateURLLoaderRequest::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const ResourceResponseHead& response_head) {
  NOTIMPLEMENTED();
}

void AppCacheUpdateURLLoaderRequest::OnDataDownloaded(
    int64_t data_len,
    int64_t encoded_data_len) {
  NOTIMPLEMENTED();
}

void AppCacheUpdateURLLoaderRequest::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback ack_callback) {
  NOTIMPLEMENTED();
}

void AppCacheUpdateURLLoaderRequest::OnReceiveCachedMetadata(
    const std::vector<uint8_t>& data) {
  NOTIMPLEMENTED();
}

void AppCacheUpdateURLLoaderRequest::OnTransferSizeUpdated(
    int32_t transfer_size_diff) {
  NOTIMPLEMENTED();
}

void AppCacheUpdateURLLoaderRequest::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  NOTIMPLEMENTED();
}

void AppCacheUpdateURLLoaderRequest::OnComplete(
    const ResourceRequestCompletionStatus& status) {
  NOTIMPLEMENTED();
}

AppCacheUpdateURLLoaderRequest::AppCacheUpdateURLLoaderRequest(
    net::URLRequestContext* request_context,
    const GURL& url,
    URLFetcher* fetcher) {}

}  // namespace content
