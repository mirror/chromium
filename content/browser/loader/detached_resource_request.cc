// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/detached_resource_request.h"

#include "base/location.h"
#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "content/browser/loader/resource_requester_info.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/referrer.h"
#include "content/public/common/service_worker_modes.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {

namespace {

void GetContextsCallback(ResourceContext* resource_context,
                         net::URLRequestContext* url_request_context,
                         content::ResourceType resource_type,
                         ResourceContext** resource_context_out,
                         net::URLRequestContext** request_context_out) {
  *request_context_out = url_request_context;
  *resource_context_out = resource_context;
}

}  // namespace

// static
std::unique_ptr<DetachedResourceRequest> DetachedResourceRequest::Create(
    BrowserContext* browser_context,
    const GURL& url,
    const GURL& site_for_cookies) {
  DCHECK(browser_context);
  auto* rdh = ResourceDispatcherHostImpl::Get();
  if (!rdh)
    return nullptr;

  auto* storage_partition =
      BrowserContext::GetStoragePartition(browser_context, nullptr);
  auto* url_request_context_getter = storage_partition->GetURLRequestContext();
  auto* resource_context = browser_context->GetResourceContext();

  return std::unique_ptr<DetachedResourceRequest>(
      new DetachedResourceRequest(rdh, url_request_context_getter,
                                  resource_context, url, site_for_cookies));
}

DetachedResourceRequest::~DetachedResourceRequest() {}

DetachedResourceRequest::DetachedResourceRequest(
    ResourceDispatcherHostImpl* rdh,
    net::URLRequestContextGetter* url_request_context_getter,
    ResourceContext* resource_context,
    const GURL& url,
    const GURL& site_for_cookies)
    : rdh_(rdh),
      url_request_context_getter_(url_request_context_getter),
      resource_context_(resource_context),
      url_(url),
      site_for_cookies_(site_for_cookies) {}

// static
void DetachedResourceRequest::Start(
    std::unique_ptr<DetachedResourceRequest> request) {
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&DetachedResourceRequest::StartOnIOThread,
                     std::move(request)));
}

void DetachedResourceRequest::StartOnIOThread() {
  net::NetworkTrafficAnnotationTag traffic_annotation = {-1};
  network::ResourceRequest resource_request;

  resource_request.method = "GET";
  resource_request.url = url_;
  resource_request.site_for_cookies = site_for_cookies_;
  resource_request.request_initiator = url::Origin::Create(site_for_cookies_);
  resource_request.referrer_policy = Referrer::GetDefaultReferrerPolicy();
  resource_request.load_flags = net::LOAD_PREFETCH;
  resource_request.resource_type = RESOURCE_TYPE_SUB_RESOURCE;
  resource_request.service_worker_mode =
      static_cast<int>(ServiceWorkerMode::NONE);
  resource_request.do_not_prompt_for_login = true;
  resource_request.render_frame_id = -1;
  resource_request.enable_load_timing = false;
  resource_request.report_raw_headers = false;
  resource_request.download_to_network_cache_only = true;

  auto* url_request_context =
      url_request_context_getter_->GetURLRequestContext();
  ResourceRequesterInfo::GetContextsCallback cb =
      base::Bind(&GetContextsCallback, base::Unretained(resource_context_),
                 base::Unretained(url_request_context));
  requester_info_ = ResourceRequesterInfo::CreateForStandaloneRequest(cb);

  rdh_->OnRequestResourceWithMojo(requester_info_.get(), -1, -1, 0,
                                  resource_request, nullptr, nullptr,
                                  traffic_annotation);
}

}  // namespace content
