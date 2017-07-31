// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/appcache_update_url_request.h"
#include "content/browser/appcache/appcache_update_url_fetcher.h"
#include "net/url_request/url_request_context.h"

namespace content {

namespace {
constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("appcache_update_job", R"(
      semantics {
        sender: "HTML5 AppCache System"
        description:
          "Web pages can include a link to a manifest file which lists "
          "resources to be cached for offline access. The AppCache system"
          "retrieves those resources in the background."
        trigger:
          "User visits a web page containing a <html manifest=manifestUrl> "
          "tag, or navigates to a document retrieved from an existing appcache "
          "and some resource should be updated."
        data: "None"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: YES
        cookies_store: "user"
        setting:
          "Users can control this feature via the 'Cookies' setting under "
          "'Privacy, Content settings'. If cookies are disabled for a single "
          "site, appcaches are disabled for the site only. If they are totally "
          "disabled, all appcache requests will be stopped."
        chrome_policy {
            DefaultCookiesSetting {
              DefaultCookiesSetting: 2
            }
          }
      })");
}

AppCacheUpdateURLRequest::~AppCacheUpdateURLRequest() {
  // To defend against URLRequest calling delegate methods during
  // destruction, we test for a !request_ in those methods.
  std::unique_ptr<net::URLRequest> temp = std::move(request_);
}

void AppCacheUpdateURLRequest::Start() {
  request_->Start();
}

void AppCacheUpdateURLRequest::SetExtraRequestHeaders(
    const net::HttpRequestHeaders& headers) {
  request_->SetExtraRequestHeaders(headers);
}

GURL AppCacheUpdateURLRequest::GetURL() const {
  return request_->url();
}

GURL AppCacheUpdateURLRequest::GetOriginalURL() const {
  return request_->original_url();
}

void AppCacheUpdateURLRequest::SetLoadFlags(int flags) {
  request_->SetLoadFlags(flags);
}

int AppCacheUpdateURLRequest::GetLoadFlags() const {
  return request_->load_flags();
}

std::string AppCacheUpdateURLRequest::GetMimeType() const {
  std::string mime_type;
  request_->GetMimeType(&mime_type);
  return mime_type;
}

void AppCacheUpdateURLRequest::SetFirstPartyForCookies(
    const GURL& first_party_for_cookies) {
  request_->set_first_party_for_cookies(first_party_for_cookies);
}

void AppCacheUpdateURLRequest::SetInitiator(
    const base::Optional<url::Origin>& initiator) {
  request_->set_initiator(initiator);
}

net::HttpResponseHeaders* AppCacheUpdateURLRequest::GetResponseHeaders() const {
  return request_->response_headers();
}

int AppCacheUpdateURLRequest::GetResponseCode() const {
  return request_->GetResponseCode();
}

net::HttpResponseInfo AppCacheUpdateURLRequest::GetResponseInfo() const {
  return request_->response_info();
}

const net::URLRequestContext* AppCacheUpdateURLRequest::GetRequestContext()
    const {
  return request_->context();
}

int AppCacheUpdateURLRequest::Read(net::IOBuffer* buf, int max_bytes) {
  return request_->Read(buf, max_bytes);
}

int AppCacheUpdateURLRequest::Cancel() {
  return request_->Cancel();
}

void AppCacheUpdateURLRequest::OnReceivedRedirect(
    net::URLRequest* request,
    const net::RedirectInfo& redirect_info,
    bool* defer_redirect) {
  if (!request_)
    return;
  DCHECK_EQ(request_.get(), request);
  fetcher_->OnReceivedRedirect(redirect_info, defer_redirect);
}

void AppCacheUpdateURLRequest::OnResponseStarted(net::URLRequest* request,
                                                 int net_error) {
  if (!request_)
    return;
  DCHECK_EQ(request_.get(), request);
  fetcher_->OnResponseStarted(net_error);
}

void AppCacheUpdateURLRequest::OnReadCompleted(net::URLRequest* request,
                                               int bytes_read) {
  if (!request_)
    return;
  DCHECK_EQ(request_.get(), request);
  fetcher_->OnReadCompleted(bytes_read);
}

AppCacheUpdateURLRequest::AppCacheUpdateURLRequest(
    net::URLRequestContext* request_context,
    const GURL& url,
    URLFetcher* fetcher)
    : request_(request_context->CreateRequest(url,
                                              net::DEFAULT_PRIORITY,
                                              this,
                                              kTrafficAnnotation)),
      fetcher_(fetcher) {}

}  // namespace content
