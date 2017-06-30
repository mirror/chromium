// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/web_service_worker_installed_scripts_manager_impl.h"

#include "base/memory/ptr_util.h"
#include "base/trace_event/trace_event.h"

namespace content {

// static
std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager>
WebServiceWorkerInstalledScriptsManagerImpl::Create() {
  TRACE_EVENT0("ServiceWorker",
               "WebServiceWorkerInstalledScriptsManager::Create");
  std::vector<GURL> installed_urls;
  return base::WrapUnique<WebServiceWorkerInstalledScriptsManagerImpl>(
      new WebServiceWorkerInstalledScriptsManagerImpl(
          std::move(installed_urls)));
}

WebServiceWorkerInstalledScriptsManagerImpl::
    WebServiceWorkerInstalledScriptsManagerImpl(
        std::vector<GURL>&& installed_urls)
    : installed_urls_(installed_urls) {}

WebServiceWorkerInstalledScriptsManagerImpl::
    ~WebServiceWorkerInstalledScriptsManagerImpl() = default;

bool WebServiceWorkerInstalledScriptsManagerImpl::IsScriptInstalled(
    const blink::WebURL& web_script_url) const {
  const GURL& script_url = web_script_url;
  bool result = false;
  for (const auto& installed_url : installed_urls_) {
    if (script_url == installed_url) {
      result = true;
      break;
    }
  }
  TRACE_EVENT2("ServiceWorker",
               "WebServiceWorkerInstalledScriptsManager::IsScriptInstalled",
               "script_url", web_script_url.GetString().Utf8(), "is installed",
               (result ? "true" : "false"));
  return result;
}

std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager::RawScriptData>
WebServiceWorkerInstalledScriptsManagerImpl::GetRawScriptData(
    const blink::WebURL& web_script_url) {
  const GURL& script_url = web_script_url;
  // TODO(shimazu): implement here.
  NOTIMPLEMENTED();
  TRACE_EVENT1("ServiceWorker",
               "WebServiceWorkerInstalledScriptsManagerImpl::GetRawScriptData",
               "script_url", script_url.spec());
  return nullptr;
}

}  // namespace content
