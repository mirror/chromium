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
    parent_->FinishScriptTransfer(script_url);
  };

 private:
  WebServiceWorkerInstalledScriptsManagerImpl* parent_;
};

}  // namespace

class WebServiceWorkerInstalledScriptsManagerImpl::ScriptData {
 public:
  ScriptData() : lock_(), waiting_cv_(&lock_) {}

  void Add(const GURL& url, std::unique_ptr<RawScriptData> data) {
    base::AutoLock lock(lock_);
    script_data_.emplace(url, std::move(data));
    if (url == waiting_url_)
      waiting_cv_.Signal();
  }

  bool Exists(const GURL& url) {
    base::AutoLock lock(lock_);
    return script_data_.find(url) != script_data_.end();
  }

  void Wait(const GURL& url) {
    base::AutoLock lock(lock_);
    if (script_data_.find(url) != script_data_.end())
      return;
    waiting_url_ = url;
    while (script_data_.find(url) == script_data_.end())
      waiting_cv_.Wait();
    waiting_url_ = GURL();
    return;
  }

  std::unique_ptr<RawScriptData> Drain(const GURL& url) {
    base::AutoLock lock(lock_);
    DCHECK(script_data_.find(url) != script_data_.end());
    auto data = std::move(script_data_[url]);
    DCHECK(script_data_[url] == nullptr);
    return data;
  }

 private:
  std::map<GURL, std::unique_ptr<RawScriptData>> script_data_;
  base::Lock lock_;
  GURL waiting_url_;
  base::ConditionVariable waiting_cv_;
};

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
    : installed_urls_(std::move(installed_urls)),
      script_data_(base::MakeUnique<ScriptData>()) {}

WebServiceWorkerInstalledScriptsManagerImpl::
    ~WebServiceWorkerInstalledScriptsManagerImpl() {
  // // Destroy the endpoint owned by |binding_| on the io thread.
  // io_task_runner_->PostTask(FROM_HERE,
  //                           base::BindOnce(&DestructOnIO,
  //                           binding_.Unbind()));
}

void WebServiceWorkerInstalledScriptsManagerImpl::FinishScriptTransfer(
    const GURL& script_url) {
  // TODO(shimazu): Create RawScriptData.
  script_data_->Add(script_url, nullptr);
}

std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager::RawScriptData>
WebServiceWorkerInstalledScriptsManagerImpl::GetRawScriptData(
    const blink::WebURL& web_script_url) {
  const GURL& script_url = web_script_url;
  bool is_found = false;
  LOG(ERROR) << __func__ << " check installed_scripts:"
             << "script_url: " << script_url.spec();
  for (const auto& installed_url : installed_urls_) {
    LOG(ERROR) << __func__ << " "
               << "script_url: " << script_url.spec() << " "
               << "installed_url: " << installed_url.spec();
    if (script_url == installed_url) {
      is_found = true;
      break;
    }
  }
  if (!is_found) {
    LOG(ERROR) << __func__ << " script was not found";
    return nullptr;
  }
  LOG(ERROR) << __func__ << " script is found on the list";

  auto contents = base::MakeUnique<RawScriptData>();
  if (!script_data_->Exists(script_url)) {
    // TODO(shimazu): Wait for arrival of the script.
    LOG(ERROR) << __func__ << " wait for arrival of the script";
    script_data_->Wait(script_url);
    LOG(ERROR) << __func__ << " script was served";
    DCHECK(script_data_->Exists(script_url));
    LOG(ERROR) << __func__ << " script was found on the container";
    return nullptr;
  }
  // Ask to the browser process when the script has already been served
  auto data = script_data_->Drain(script_url);
  if (!data) {
    LOG(ERROR) << __func__ << " script was not found";
    return nullptr;
  }
  LOG(ERROR) << __func__ << " script should be served";
  return script_data_->Drain(script_url);
}

}  // namespace content
