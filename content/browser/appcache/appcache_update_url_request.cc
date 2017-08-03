// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/appcache_update_url_request.h"
#include "content/browser/appcache/appcache_update_url_fetcher.h"
#include "net/url_request/url_request_context.h"

namespace content {

AppCacheUpdateJob::UpdateURLRequest::~UpdateURLRequest() {
  // To defend against URLRequest calling delegate methods during
  // destruction, we test for a !request_ in those methods.
  std::unique_ptr<net::URLRequest> temp = std::move(request_);
}

void AppCacheUpdateJob::UpdateURLRequest::Start() {
  request_->Start();
}

void AppCacheUpdateJob::UpdateURLRequest::SetExtraRequestHeaders(
    const net::HttpRequestHeaders& headers) {
  request_->SetExtraRequestHeaders(headers);
}

GURL AppCacheUpdateJob::UpdateURLRequest::GetURL() const {
  return request_->url();
}

GURL AppCacheUpdateJob::UpdateURLRequest::GetOriginalURL() const {
  return request_->original_url();
}

void AppCacheUpdateJob::UpdateURLRequest::SetLoadFlags(int flags) {
  request_->SetLoadFlags(flags);
}

int AppCacheUpdateJob::UpdateURLRequest::GetLoadFlags() const {
  return request_->load_flags();
}

std::string AppCacheUpdateJob::UpdateURLRequest::GetMimeType() const {
  std::string mime_type;
  request_->GetMimeType(&mime_type);
  return mime_type;
}

void AppCacheUpdateJob::UpdateURLRequest::SetFirstPartyForCookies(
    const GURL& first_party_for_cookies) {
  request_->set_first_party_for_cookies(first_party_for_cookies);
}

void AppCacheUpdateJob::UpdateURLRequest::SetInitiator(
    const base::Optional<url::Origin>& initiator) {
  request_->set_initiator(initiator);
}

net::HttpResponseHeaders*
AppCacheUpdateJob::UpdateURLRequest::GetResponseHeaders() const {
  return request_->response_headers();
}

int AppCacheUpdateJob::UpdateURLRequest::GetResponseCode() const {
  return request_->GetResponseCode();
}

net::HttpResponseInfo AppCacheUpdateJob::UpdateURLRequest::GetResponseInfo()
    const {
  return request_->response_info();
}

const net::URLRequestContext*
AppCacheUpdateJob::UpdateURLRequest::GetRequestContext() const {
  return request_->context();
}

int AppCacheUpdateJob::UpdateURLRequest::Read(net::IOBuffer* buf,
                                              int max_bytes) {
  return request_->Read(buf, max_bytes);
}

int AppCacheUpdateJob::UpdateURLRequest::Cancel() {
  return request_->Cancel();
}

void AppCacheUpdateJob::UpdateURLRequest::OnReceivedRedirect(
    net::URLRequest* request,
    const net::RedirectInfo& redirect_info,
    bool* defer_redirect) {
  if (!request_)
    return;
  DCHECK_EQ(request_.get(), request);
  fetcher_->OnReceivedRedirect(redirect_info);
}

void AppCacheUpdateJob::UpdateURLRequest::OnResponseStarted(
    net::URLRequest* request,
    int net_error) {
  if (!request_)
    return;
  DCHECK_EQ(request_.get(), request);
  fetcher_->OnResponseStarted(net_error);
}

void AppCacheUpdateJob::UpdateURLRequest::OnReadCompleted(
    net::URLRequest* request,
    int bytes_read) {
  if (!request_)
    return;
  DCHECK_EQ(request_.get(), request);
  fetcher_->OnReadCompleted(bytes_read);
}

AppCacheUpdateJob::UpdateURLRequest::UpdateURLRequest(
    net::URLRequestContext* request_context,
    const GURL& url,
    URLFetcher* fetcher)
    : request_(request_context->CreateRequest(url,
                                              net::DEFAULT_PRIORITY,
                                              this,
                                              GetTrafficAnnotation())),
      fetcher_(fetcher) {}

}  // namespace content
