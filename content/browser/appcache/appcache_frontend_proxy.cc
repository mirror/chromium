// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/appcache_frontend_proxy.h"

#include "content/common/appcache.mojom.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/bind_interface_helpers.h"

namespace content {

AppCacheFrontendProxy::AppCacheFrontendProxy(
    RenderProcessHost* render_process_host)
    : render_process_host_(render_process_host) {}

AppCacheFrontendProxy::~AppCacheFrontendProxy() {}

mojom::AppCacheRenderer* AppCacheFrontendProxy::GetAppCacheRenderer() {
  if (!app_cache_renderer_ptr_) {
    BindInterface(render_process_host_, &app_cache_renderer_ptr_);
  }
  return app_cache_renderer_ptr_.get();
}

void AppCacheFrontendProxy::OnCacheSelected(
    int host_id, const AppCacheInfo& info) {
  GetAppCacheRenderer()->CacheSelected(host_id, info.Clone());
}

void AppCacheFrontendProxy::OnStatusChanged(const std::vector<int>& host_ids,
                                            AppCacheStatus status) {
  GetAppCacheRenderer()->StatusChanged(host_ids, status);
}

void AppCacheFrontendProxy::OnEventRaised(const std::vector<int>& host_ids,
                                          AppCacheEventID event_id) {
  DCHECK_NE(AppCacheEventID::APPCACHE_PROGRESS_EVENT,
            event_id);  // See OnProgressEventRaised.
  GetAppCacheRenderer()->EventRaised(host_ids, event_id);
}

void AppCacheFrontendProxy::OnProgressEventRaised(
    const std::vector<int>& host_ids,
    const GURL& url, int num_total, int num_complete) {
  GetAppCacheRenderer()->ProgressEventRaised(host_ids, url, num_total,
                                             num_complete);
}

void AppCacheFrontendProxy::OnErrorEventRaised(
    const std::vector<int>& host_ids,
    const AppCacheErrorDetails& details) {
  GetAppCacheRenderer()->ErrorEventRaised(host_ids, details.Clone());
}

void AppCacheFrontendProxy::OnLogMessage(int host_id,
                                         AppCacheLogLevel log_level,
                                         const std::string& message) {
  GetAppCacheRenderer()->LogMessage(host_id, log_level, message);
}

void AppCacheFrontendProxy::OnContentBlocked(int host_id,
                                             const GURL& manifest_url) {
  GetAppCacheRenderer()->ContentBlocked(host_id, manifest_url);
}

void AppCacheFrontendProxy::OnSetSubresourceFactory(
    int host_id,
    mojom::URLLoaderFactoryPtr url_loader_factory) {
  GetAppCacheRenderer()->SetSubresourceFactory(host_id,
                                               std::move(url_loader_factory));
}

}  // namespace content
