// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_installed_scripts_sender.h"

#include "content/browser/service_worker/service_worker_context_core.h"
#include "content/browser/service_worker/service_worker_script_cache_map.h"

namespace content {

ServiceWorkerInstalledScriptsSender::ServiceWorkerInstalledScriptsSender(
    const GURL& main_script_url,
    base::WeakPtr<ServiceWorkerScriptCacheMap> cache_map,
    base::WeakPtr<ServiceWorkerContextCore> context)
    : main_script_url_(main_script_url),
      cache_map_(std::move(cache_map)),
      context_(std::move(context)) {}

ServiceWorkerInstalledScriptsSender::~ServiceWorkerInstalledScriptsSender() {}

mojom::ServiceWorkerInstalledScriptsInfoPtr
ServiceWorkerInstalledScriptsSender::CreateInfoAndBind() {
  DCHECK(cache_map_);
  auto info = mojom::ServiceWorkerInstalledScriptsInfo::New();
  info->manager_request = mojo::MakeRequest(&manager_);

  // Read all installed urls.
  std::vector<ServiceWorkerDatabase::ResourceRecord> resources;
  cache_map_->GetResources(&resources);
  for (const auto& resource : resources) {
    info->installed_urls.emplace_back(resource.url);
  }
  // Start pushing the data from the main script.

  return info;
}

}  // namespace content
