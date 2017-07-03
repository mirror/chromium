// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/web_service_worker_installed_scripts_manager_impl.h"

#include "base/trace_event/trace_event.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

namespace {

class Internal : public mojom::ServiceWorkerInstalledScriptsManager {
 public:
  // Called on IO
  static void Create(mojom::ServiceWorkerInstalledScriptsManagerRequest request,
                     base::WaitableEvent* event) {
    TRACE_EVENT0("ServiceWorker",
                 "ServiceWorkerInstalledScriptsManager::Internal");
    mojo::MakeStrongBinding(base::MakeUnique<Internal>(), std::move(request));
    event->Signal();
  }

  // Implements mojom::ServiceWorkerInstalledScriptsManager.
  // Called on the io thread.
  void TransferInstalledScript(
      const GURL& script_url,
      const std::string& encoding,
      const std::unordered_map<std::string, std::string>& headers,
      mojo::ScopedDataPipeConsumerHandle body,
      mojo::ScopedDataPipeConsumerHandle meta_data) override {}
};

}  // namespace

// static
std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager>
WebServiceWorkerInstalledScriptsManagerImpl::Create(
    mojom::ServiceWorkerInstalledScriptsInfoPtr installed_scripts_info,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner) {
  TRACE_EVENT0("ServiceWorker",
               "WebServiceWorkerInstalledScriptsManager::Create");
  auto installed_scripts_manager =
      base::WrapUnique<WebServiceWorkerInstalledScriptsManagerImpl>(
          new WebServiceWorkerInstalledScriptsManagerImpl(
              std::move(installed_scripts_info->installed_urls)));
  // TODO(shimazu): Don't use waitable event here.
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  // TODO(shimazu): Pass |this| as parent
  io_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&Internal::Create,
                     std::move(installed_scripts_info->manager_request),
                     &event));
  event.Wait();
  return base::WrapUnique(installed_scripts_manager.release());
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
