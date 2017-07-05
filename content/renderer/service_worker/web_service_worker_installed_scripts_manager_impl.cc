// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/web_service_worker_installed_scripts_manager_impl.h"

#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

namespace {

class Internal : public mojom::ServiceWorkerInstalledScriptsManager {
 public:
  // Called on IO
  static void Create(
      mojom::ServiceWorkerInstalledScriptsManagerRequest request) {
    mojo::MakeStrongBinding(base::MakeUnique<Internal>(), std::move(request));
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
void WebServiceWorkerInstalledScriptsManagerImpl::Create(
    mojom::ServiceWorkerInstalledScriptsInfoPtr installed_scripts_info,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
    CreatedOnceCallback callback) {
  // TODO(shimazu): Pass |this| to Internal::Create as parent
  io_task_runner->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&Internal::Create,
                     std::move(installed_scripts_info->manager_request)),
      base::BindOnce(
          &WebServiceWorkerInstalledScriptsManagerImpl::OnCreatedInternal,
          base::Passed(&installed_scripts_info->installed_urls),
          base::Passed(&callback)));
}

// static
void WebServiceWorkerInstalledScriptsManagerImpl::OnCreatedInternal(
    std::vector<GURL> installed_urls,
    CreatedOnceCallback callback) {
  auto installed_scripts_manager =
      base::WrapUnique<WebServiceWorkerInstalledScriptsManagerImpl>(
          new WebServiceWorkerInstalledScriptsManagerImpl(
              std::move(installed_urls)));
  std::move(callback).Run(std::move(installed_scripts_manager));
}

WebServiceWorkerInstalledScriptsManagerImpl::
    WebServiceWorkerInstalledScriptsManagerImpl(
        std::vector<GURL>&& installed_urls)
    : installed_urls_(installed_urls.begin(), installed_urls.end()) {}

WebServiceWorkerInstalledScriptsManagerImpl::
    ~WebServiceWorkerInstalledScriptsManagerImpl() = default;

bool WebServiceWorkerInstalledScriptsManagerImpl::IsScriptInstalled(
    const blink::WebURL& web_script_url) const {
  return base::ContainsKey(installed_urls_, web_script_url);
}

std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager::RawScriptData>
WebServiceWorkerInstalledScriptsManagerImpl::GetRawScriptData(
    const blink::WebURL& web_script_url) {
  // TODO(shimazu): Implement here.
  NOTIMPLEMENTED();
  return nullptr;
}

}  // namespace content
