// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/service_worker_fetch_context_impl.h"

#include "base/feature_list.h"
#include "content/child/request_extra_data.h"
#include "content/child/resource_dispatcher.h"
#include "content/child/web_url_loader_impl.h"
#include "content/public/common/content_features.h"

namespace content {

ServiceWorkerFetchContextImpl::ServiceWorkerFetchContextImpl(
    const GURL& worker_script_url,
    mojom::URLLoaderFactoryPtrInfo net_url_loader_factory_info,
    mojom::URLLoaderFactoryPtrInfo blob_url_loader_factory_info,
    int service_worker_provider_id)
    : worker_script_url_(worker_script_url),
      net_url_loader_factory_info_(std::move(net_url_loader_factory_info)),
      blob_url_loader_factory_info_(std::move(blob_url_loader_factory_info)),
      service_worker_provider_id_(service_worker_provider_id) {}

ServiceWorkerFetchContextImpl::~ServiceWorkerFetchContextImpl() {}

void ServiceWorkerFetchContextImpl::InitializeOnWorkerThread(
    base::SingleThreadTaskRunner* loading_task_runner) {
  resource_dispatcher_ =
      base::MakeUnique<ResourceDispatcher>(nullptr, loading_task_runner);
  DCHECK(net_url_loader_factory_info_.is_valid());
  net_url_loader_factory_.Bind(std::move(net_url_loader_factory_info_));

  if (base::FeatureList::IsEnabled(features::kNetworkService)) {
    DCHECK(blob_url_loader_factory_info_.is_valid());
    blob_url_loader_factory_.Bind(std::move(blob_url_loader_factory_info_));
  }
}

std::unique_ptr<blink::WebURLLoader>
ServiceWorkerFetchContextImpl::CreateURLLoader(
    const blink::WebURLRequest& request,
    base::SingleThreadTaskRunner* task_runner) {
  if (base::FeatureList::IsEnabled(features::kNetworkService) &&
      request.Url().ProtocolIs(url::kBlobScheme)) {
    DCHECK(blob_url_loader_factory_);
    return base::MakeUnique<content::WebURLLoaderImpl>(
        resource_dispatcher_.get(), task_runner,
        blob_url_loader_factory_.get());
  }
  DCHECK(net_url_loader_factory_);
  return base::MakeUnique<content::WebURLLoaderImpl>(
      resource_dispatcher_.get(), task_runner, net_url_loader_factory_.get());
}

void ServiceWorkerFetchContextImpl::WillSendRequest(
    blink::WebURLRequest& request) {
  RequestExtraData* extra_data = new RequestExtraData();
  extra_data->set_service_worker_provider_id(service_worker_provider_id_);
  extra_data->set_originated_from_service_worker(true);
  extra_data->set_initiated_in_secure_context(true);
  request.SetExtraData(extra_data);
}

bool ServiceWorkerFetchContextImpl::IsControlledByServiceWorker() const {
  return false;
}

void ServiceWorkerFetchContextImpl::SetDataSaverEnabled(bool enabled) {
  is_data_saver_enabled_ = enabled;
}

bool ServiceWorkerFetchContextImpl::IsDataSaverEnabled() const {
  return is_data_saver_enabled_;
}

blink::WebURL ServiceWorkerFetchContextImpl::SiteForCookies() const {
  // According to the spec, we can use the |worker_script_url_| for
  // SiteForCookies, because "site for cookies" for the service worker is
  // the service worker's origin's host's registrable domain.
  // https://tools.ietf.org/html/draft-ietf-httpbis-cookie-same-site-07#section-2.1.2
  return worker_script_url_;
}

}  // namespace content
