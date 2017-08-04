// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/appcache_update_url_loader_request.h"
#include "content/browser/appcache/appcache_update_url_fetcher.h"
#include "net/http/http_response_info.h"
#include "net/url_request/url_request_context.h"

namespace content {

AppCacheUpdateJob::UpdateURLLoaderRequest::~UpdateURLLoaderRequest() {}

void AppCacheUpdateJob::UpdateURLLoaderRequest::Start() {
  mojom::URLLoaderClientPtr client;
  client_binding_.Bind(mojo::MakeRequest(&client));

  loader_factory_getter_->GetNetworkFactory()->get()->CreateLoaderAndStart(
      mojo::MakeRequest(&url_loader_), -1, -1, 0, request_, std::move(client),
      net::MutableNetworkTrafficAnnotationTag(GetTrafficAnnotation()));
}

void AppCacheUpdateJob::UpdateURLLoaderRequest::SetExtraRequestHeaders(
    const net::HttpRequestHeaders& headers) {
  request_.headers = headers.ToString();
}

GURL AppCacheUpdateJob::UpdateURLLoaderRequest::GetURL() const {
  return request_.url;
}

void AppCacheUpdateJob::UpdateURLLoaderRequest::SetLoadFlags(int flags) {
  request_.load_flags = flags;
}

int AppCacheUpdateJob::UpdateURLLoaderRequest::GetLoadFlags() const {
  return request_.load_flags;
}

std::string AppCacheUpdateJob::UpdateURLLoaderRequest::GetMimeType() const {
  return response_->mime_type;
}

void AppCacheUpdateJob::UpdateURLLoaderRequest::SetFirstPartyForCookies(
    const GURL& first_party_for_cookies) {
  request_.first_party_for_cookies = first_party_for_cookies;
}

void AppCacheUpdateJob::UpdateURLLoaderRequest::SetInitiator(
    const base::Optional<url::Origin>& initiator) {
  request_.request_initiator = initiator;
}

net::HttpResponseHeaders*
AppCacheUpdateJob::UpdateURLLoaderRequest::GetResponseHeaders() const {
  return response_->headers.get();
}

int AppCacheUpdateJob::UpdateURLLoaderRequest::GetResponseCode() const {
  if (response_->headers)
    return response_->headers->response_code();
  return 0;
}

net::HttpResponseInfo
AppCacheUpdateJob::UpdateURLLoaderRequest::GetResponseInfo() const {
  return *http_response_info_;
}

void AppCacheUpdateJob::UpdateURLLoaderRequest::Read() {
  read_requested_ = true;
  // Initiate a read from the pipe if we have not done so.
  MaybeStartReading();
}

int AppCacheUpdateJob::UpdateURLLoaderRequest::Cancel() {
  client_binding_.Close();
  url_loader_ = nullptr;
  return 0;
}

// TODO(ananta/michaeln)
// Add support in the URLLoader to return information on whether the http stack
// is configured to ignore certificate errors.
bool AppCacheUpdateJob::UpdateURLLoaderRequest::ShouldIgnoreCertificateErrors()
    const {
  return false;
}

void AppCacheUpdateJob::UpdateURLLoaderRequest::OnReceiveResponse(
    const ResourceResponseHead& response_head,
    const base::Optional<net::SSLInfo>& ssl_info,
    mojom::DownloadedTempFilePtr downloaded_file) {
  response_.reset(new ResourceResponseHead());
  *response_ = response_head;

  // TODO(ananta/michaeln)
  // Populate other fields in the HttpResponseInfo class. It would be good to
  // have a helper function which populates the HttpResponseInfo structure from
  // the ResourceResponseHead structure.
  http_response_info_.reset(new net::HttpResponseInfo());
  if (ssl_info.has_value())
    http_response_info_->ssl_info = *ssl_info;
  http_response_info_->headers = response_head.headers;
  http_response_info_->was_fetched_via_spdy =
      response_head.was_fetched_via_spdy;
  http_response_info_->was_alpn_negotiated = response_head.was_alpn_negotiated;
  http_response_info_->alpn_negotiated_protocol =
      response_head.alpn_negotiated_protocol;
  http_response_info_->connection_info = response_head.connection_info;
  http_response_info_->socket_address = response_head.socket_address;
  fetcher_->OnResponseStarted(net::OK);
}

void AppCacheUpdateJob::UpdateURLLoaderRequest::OnReceiveRedirect(
    const net::RedirectInfo& redirect_info,
    const ResourceResponseHead& response_head) {
  *response_ = response_head;
  fetcher_->OnReceivedRedirect(redirect_info);
}

void AppCacheUpdateJob::UpdateURLLoaderRequest::OnDataDownloaded(
    int64_t data_len,
    int64_t encoded_data_len) {
  NOTIMPLEMENTED();
}

void AppCacheUpdateJob::UpdateURLLoaderRequest::OnUploadProgress(
    int64_t current_position,
    int64_t total_size,
    OnUploadProgressCallback ack_callback) {
  NOTIMPLEMENTED();
}

void AppCacheUpdateJob::UpdateURLLoaderRequest::OnReceiveCachedMetadata(
    const std::vector<uint8_t>& data) {
}

void AppCacheUpdateJob::UpdateURLLoaderRequest::OnTransferSizeUpdated(
    int32_t transfer_size_diff) {
  NOTIMPLEMENTED();
}

void AppCacheUpdateJob::UpdateURLLoaderRequest::OnStartLoadingResponseBody(
    mojo::ScopedDataPipeConsumerHandle body) {
  handle_ = std::move(body);
  // Initiate a read from the pipe if we have a pending Read() request.
  MaybeStartReading();
}

void AppCacheUpdateJob::UpdateURLLoaderRequest::OnComplete(
    const ResourceRequestCompletionStatus& status) {
  response_status_ = status;
  // We inform the URLFetcher about a failure only here. For the success case
  // OnResponseCompleted() is invoked URLFetcher::OnReadCompleted().
  if (status.error_code != net::OK)
    fetcher_->OnResponseCompleted(status.error_code);
}

AppCacheUpdateJob::UpdateURLLoaderRequest::UpdateURLLoaderRequest(
    URLLoaderFactoryGetter* loader_factory_getter,
    const GURL& url,
    int buffer_size,
    URLFetcher* fetcher)
    : fetcher_(fetcher),
      loader_factory_getter_(loader_factory_getter),
      client_binding_(this),
      buffer_(new net::IOBuffer(buffer_size)),
      buffer_size_(buffer_size),
      handle_watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL),
      read_requested_(false) {
  request_.url = url;
  request_.method = "GET";
}

int AppCacheUpdateJob::UpdateURLLoaderRequest::OnDataAvailable(
    size_t num_bytes) {
  int bytes_copied = std::min<int>(buffer_size_, num_bytes);

  scoped_refptr<MojoToNetIOBuffer> buffer =
      new MojoToNetIOBuffer(pending_read_.get());

  read_requested_ = false;
  fetcher_->OnReadCompleted(buffer.get(), bytes_copied);
  return bytes_copied;
}

void AppCacheUpdateJob::UpdateURLLoaderRequest::OnDataComplete() {
  DCHECK(response_status_.error_code == net::OK);
  fetcher_->OnReadCompleted(buffer_.get(), 0);
}

void AppCacheUpdateJob::UpdateURLLoaderRequest::StartReading(
    MojoResult unused) {
  // Wait for the caller to initiate a read.
  if (!read_requested_)
    return;

  uint32_t bytes_read = 0;
  MojoResult result =
      MojoToNetPendingBuffer::BeginRead(&handle_, &pending_read_, &bytes_read);
  if (result == MOJO_RESULT_SHOULD_WAIT) {
    handle_watcher_.ArmOrNotify();
    return;
  }

  if (result == MOJO_RESULT_BUSY)
    return;

  if (result == MOJO_RESULT_FAILED_PRECONDITION) {
    OnDataComplete();
    return;
  }

  if (result != MOJO_RESULT_OK) {
    response_status_.error_code = net::ERR_FAILED;
    OnComplete(response_status_);
    return;
  }

  int bytes_copied = OnDataAvailable(bytes_read);
  handle_ = pending_read_->Complete(bytes_copied);
}

void AppCacheUpdateJob::UpdateURLLoaderRequest::MaybeStartReading() {
  if (!handle_.is_valid() || !read_requested_)
    return;

  if (handle_watcher_.IsWatching()) {
    handle_watcher_.ArmOrNotify();
    return;
  }

  handle_watcher_.Watch(
      handle_.get(), MOJO_HANDLE_SIGNAL_READABLE,
      base::Bind(&AppCacheUpdateJob::UpdateURLLoaderRequest::StartReading,
                 base::Unretained(this)));
  StartReading(0);
}

}  // namespace content
