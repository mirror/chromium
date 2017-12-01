// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/appcache/appcache_dispatcher_host.h"

#include <map>
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "content/browser/appcache/appcache_navigation_handle_core.h"
#include "content/browser/appcache/chrome_appcache_service.h"
#include "content/browser/bad_message.h"
#include "content/common/appcache_messages.h"
#include "content/public/common/browser_side_navigation_policy.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

AppCacheDispatcherHost::AppCacheDispatcherHost(
    ChromeAppCacheService* appcache_service,
    int process_id)
    : appcache_service_(appcache_service),
      frontend_proxy_(this),
      process_id_(process_id),
      weak_factory_(this) {}

void AppCacheDispatcherHost::InitBackend() {
  if (!appcache_service_.get())
    return;

  backend_impl_.Initialize(appcache_service_.get(), &frontend_proxy_,
                           process_id_);
}

AppCacheDispatcherHost::~AppCacheDispatcherHost() {}

void AppCacheDispatcherHost::Create(ChromeAppCacheService* appcache_service,
                                    int process_id,
                                    mojom::AppCacheHostRequest request) {
  auto appcache_dispatcher_host =
      std::make_unique<AppCacheDispatcherHost>(appcache_service, process_id);

  appcache_dispatcher_host->InitBackend();

  mojo::MakeStrongBinding(std::move(appcache_dispatcher_host),
                          std::move(request));
}

bool AppCacheDispatcherHost::Send(IPC::Message* msg) {
  return false;
}

void AppCacheDispatcherHost::RegisterHost(int32_t host_id) {
  if (appcache_service_.get()) {
    // PlzNavigate
    // The AppCacheHost could have been precreated in which case we want to
    // register it with the backend here.
    if (IsBrowserSideNavigationEnabled()) {
      std::unique_ptr<content::AppCacheHost> host =
          AppCacheNavigationHandleCore::GetPrecreatedHost(host_id);
      if (host.get()) {
        backend_impl_.RegisterPrecreatedHost(std::move(host));
        return;
      }
    }
    if (!backend_impl_.RegisterHost(host_id)) {
      mojo::ReportBadMessage("ACDH_REGISTER");
    }
  }
}

void AppCacheDispatcherHost::UnregisterHost(int32_t host_id) {
  if (appcache_service_.get()) {
    if (!backend_impl_.UnregisterHost(host_id)) {
      mojo::ReportBadMessage("ACDH_UNREGISTER");
    }
  }
}

void AppCacheDispatcherHost::SetSpawningHostId(int32_t host_id,
                                               int spawning_host_id) {
  if (appcache_service_.get()) {
    if (!backend_impl_.SetSpawningHostId(host_id, spawning_host_id))
      mojo::ReportBadMessage("ACDH_SET_SPAWNING");
  }
}

void AppCacheDispatcherHost::SelectCache(int32_t host_id,
                                         const GURL& document_url,
                                         int64_t cache_document_was_loaded_from,
                                         const GURL& opt_manifest_url) {
  if (appcache_service_.get()) {
    if (!backend_impl_.SelectCache(host_id, document_url,
                                   cache_document_was_loaded_from,
                                   opt_manifest_url)) {
      mojo::ReportBadMessage("ACDH_SELECT_CACHE");
    }
  } else {
    frontend_proxy_.OnCacheSelected(host_id, AppCacheInfo());
  }
}

void AppCacheDispatcherHost::SelectCacheForSharedWorker(int32_t host_id,
                                                        int64_t appcache_id) {
  if (appcache_service_.get()) {
    if (!backend_impl_.SelectCacheForSharedWorker(host_id, appcache_id))
      mojo::ReportBadMessage("ACDH_SELECT_CACHE_FOR_SHARED_WORKER");
  } else {
    frontend_proxy_.OnCacheSelected(host_id, AppCacheInfo());
  }
}

void AppCacheDispatcherHost::MarkAsForeignEntry(
    int32_t host_id,
    const GURL& document_url,
    int64_t cache_document_was_loaded_from) {
  if (appcache_service_.get()) {
    if (!backend_impl_.MarkAsForeignEntry(host_id, document_url,
                                          cache_document_was_loaded_from)) {
      mojo::ReportBadMessage("ACDH_MARK_AS_FOREIGN_ENTRY");
    }
  }
}

void AppCacheDispatcherHost::GetResourceList(int32_t host_id,
                                             GetResourceListCallback callback) {
  std::vector<AppCacheResourceInfo> params;
  std::vector<mojom::AppCacheResourceInfoPtr> out;
  if (appcache_service_.get()) {
    backend_impl_.GetResourceList(host_id, &params);

    // Box up params for output.
    out.reserve(params.size());
    for (const auto& p : params) {
      out.emplace_back(p.Clone());
    }
  }
  std::move(callback).Run(std::move(out));
}

template <typename Callback, typename Arg>
void RunAndIgnoreLastParam(Callback cb, Arg arg, void* ignored) {
  std::move(cb).Run(std::move(arg));
}

void AppCacheDispatcherHost::GetStatus(int32_t host_id,
                                       GetStatusCallback callback) {
  if (appcache_service_.get()) {
    if (!backend_impl_.GetStatusWithCallback(
            host_id,
            base::BindOnce(
                &RunAndIgnoreLastParam<GetStatusCallback, AppCacheStatus>,
                std::move(callback)),
            nullptr)) {
      mojo::ReportBadMessage("ACDH_GET_STATUS");
    }
    return;
  }

  std::move(callback).Run(AppCacheStatus::APPCACHE_STATUS_UNCACHED);
}

void AppCacheDispatcherHost::StartUpdate(int32_t host_id,
                                         StartUpdateCallback callback) {
  if (appcache_service_.get()) {
    if (!backend_impl_.StartUpdateWithCallback(
            host_id,
            base::BindOnce(&RunAndIgnoreLastParam<StartUpdateCallback, bool>,
                           std::move(callback)),
            nullptr)) {
      mojo::ReportBadMessage("ACDH_START_UPDATE");
    }
    return;
  }

  std::move(callback).Run(false);
}

void AppCacheDispatcherHost::SwapCache(int32_t host_id,
                                       SwapCacheCallback callback) {
  if (appcache_service_.get()) {
    if (!backend_impl_.SwapCacheWithCallback(
            host_id,
            base::BindOnce(&RunAndIgnoreLastParam<SwapCacheCallback, bool>,
                           std::move(callback)),
            nullptr)) {
      mojo::ReportBadMessage("ACDH_SWAP_CACHE");
    }
    return;
  }

  std::move(callback).Run(false);
}

}  // namespace content
