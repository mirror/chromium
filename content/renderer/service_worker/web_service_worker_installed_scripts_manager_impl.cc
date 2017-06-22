// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/web_service_worker_installed_scripts_manager_impl.h"

#include "base/synchronization/waitable_event.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

namespace {

class Internal : public mojom::ServiceWorkerInstalledScriptsManager {
 public:
  // Called on IO
  static void Create(WebServiceWorkerInstalledScriptsManagerImpl* parent,
                     mojom::ServiceWorkerInstalledScriptsManagerRequest request,
                     base::WaitableEvent* event) {
    mojo::MakeStrongBinding(base::MakeUnique<Internal>(parent),
                            std::move(request));
    event->Signal();
  }

  Internal(WebServiceWorkerInstalledScriptsManagerImpl* parent)
      : parent_(parent) {}

  // Implements mojom::ServiceWorkerInstalledScriptsManager.
  // Called on the io thread.
  void TransferInstalledScript(
      const GURL& script_url,
      const std::string& encoding,
      uint32_t body_size,
      mojo::ScopedDataPipeConsumerHandle body,
      uint32_t meta_data_size,
      mojo::ScopedDataPipeConsumerHandle meta_data) override {
    parent_->FinishScriptTransfer();
  };

 private:
  WebServiceWorkerInstalledScriptsManagerImpl* parent_;
};

}  // namespace

// static
std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager>
WebServiceWorkerInstalledScriptsManagerImpl::Create(
    mojom::ServiceWorkerInstalledScriptsInfoPtr installed_scripts_info,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner) {
  auto installed_scripts_manager =
      base::WrapUnique<WebServiceWorkerInstalledScriptsManagerImpl>(
          new WebServiceWorkerInstalledScriptsManagerImpl(
              std::move(installed_scripts_info->installed_urls)));
  // TODO(shimazu): Don't use waitable event here.
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  io_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&Internal::Create, installed_scripts_manager.get(),
                     std::move(installed_scripts_info->manager_request),
                     &event));
  event.Wait();
  return base::WrapUnique(installed_scripts_manager.release());
}

WebServiceWorkerInstalledScriptsManagerImpl::
    WebServiceWorkerInstalledScriptsManagerImpl(
        std::vector<GURL> installed_urls)
    : installed_urls_(std::move(installed_urls)) {}

WebServiceWorkerInstalledScriptsManagerImpl::
    ~WebServiceWorkerInstalledScriptsManagerImpl() {
  // // Destroy the endpoint owned by |binding_| on the io thread.
  // io_task_runner_->PostTask(FROM_HERE,
  //                           base::BindOnce(&DestructOnIO,
  //                           binding_.Unbind()));
}

void WebServiceWorkerInstalledScriptsManagerImpl::InitializeOnIO() {}

std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager::RawScriptData>
WebServiceWorkerInstalledScriptsManagerImpl::GetRawScriptData(
    const blink::WebURL& web_script_url) {
  const GURL& script_url = web_script_url;
  bool is_found = false;
  for (const auto& installed_url : installed_urls_) {
    LOG(ERROR) << __func__ << " "
               << "script_url: " << script_url.spec() << " "
               << "installed_url: " << installed_url.spec();
    if (script_url == installed_url) {
      is_found = true;
      break;
    }
  }
  if (!is_found)
    return nullptr;

  auto contents = base::MakeUnique<RawScriptData>();
  if (meta_data.find(script_url) == meta_data.end()) {
    // Wait for arrival of the script.
    return nullptr;
  }
  // Ask to the browser process when the script has already been served.
  if (!meta_data[script_url])
    return nullptr;
  return std::move(meta_data[script_url]);
}

}  // namespace content
