// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/web_service_worker_installed_scripts_manager_impl.h"

#include "base/synchronization/waitable_event.h"
#include "base/trace_event/trace_event.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

namespace {

class Receiver {
 public:
  Receiver(mojo::ScopedDataPipeConsumerHandle handle)
      : handle_(std::move(handle)),
        watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL) {}

  void Start(base::Closure callback) {
    callback_ = callback;
    watcher_.Watch(handle_.get(), MOJO_HANDLE_SIGNAL_READABLE,
                   base::Bind(&Receiver::OnReadable, base::Unretained(this)));
    watcher_.ArmOrNotify();
  }

  void OnReadable(MojoResult /* unused */) {
    const void* buffer = nullptr;
    uint32_t available = 0;
    MojoResult rv = mojo::BeginReadDataRaw(handle_.get(), &buffer, &available,
                                           MOJO_READ_DATA_FLAG_NONE);
    LOG(ERROR) << __PRETTY_FUNCTION__ << " available:" << available;
    switch (rv) {
      case MOJO_RESULT_BUSY:
      case MOJO_RESULT_FAILED_PRECONDITION:
      case MOJO_RESULT_INVALID_ARGUMENT:
        // Closed by peer.
        watcher_.Cancel();
        handle_.reset();
        callback_.Run();
        return;
      case MOJO_RESULT_SHOULD_WAIT:
        watcher_.ArmOrNotify();
        return;
    }
    DCHECK_EQ(MOJO_RESULT_OK, rv);
    if (available > 0) {
      data_.emplace_back(static_cast<const char*>(buffer), available);
    }
    rv = mojo::EndReadDataRaw(handle_.get(), available);
    if (rv != MOJO_RESULT_OK) {
      // Something goes bad.
      watcher_.Cancel();
      handle_.reset();
      callback_.Run();
      return;
    }
    watcher_.ArmOrNotify();
  }

  blink::WebVector<blink::WebVector<char>> DrainData() {
    return blink::WebVector<blink::WebVector<char>>(std::move(data_));
  }

  bool is_running() { return handle_.is_valid(); }

 private:
  base::Closure callback_;
  mojo::ScopedDataPipeConsumerHandle handle_;

  mojo::SimpleWatcher watcher_;

  std::vector<blink::WebVector<char>> data_;
};

class Internal : public mojom::ServiceWorkerInstalledScriptsManager {
 public:
  // Called on IO
  static void Create(WebServiceWorkerInstalledScriptsManagerImpl* parent,
                     mojom::ServiceWorkerInstalledScriptsManagerRequest request,
                     base::WaitableEvent* event) {
    TRACE_EVENT0("ServiceWorker",
                 "ServiceWorkerInstalledScriptsManager::Internal");
    mojo::MakeStrongBinding(base::MakeUnique<Internal>(parent),
                            std::move(request));
    event->Signal();
  }

  Internal(WebServiceWorkerInstalledScriptsManagerImpl* parent)
      : parent_(parent), weak_factory_(this) {}

  // Implements mojom::ServiceWorkerInstalledScriptsManager.
  // Called on the io thread.
  void TransferInstalledScript(
      const GURL& script_url,
      const std::string& encoding,
      mojo::ScopedDataPipeConsumerHandle body,
      mojo::ScopedDataPipeConsumerHandle meta_data) override {
    LOG(ERROR) << __PRETTY_FUNCTION__ << " " << script_url.spec();
    auto receivers =
        base::MakeUnique<Receivers>(std::move(meta_data), std::move(body));
    receivers->Start(base::Bind(&Internal::OnScriptReceived,
                                weak_factory_.GetWeakPtr(), script_url,
                                encoding));
    running_receivers_[script_url] = std::move(receivers);
  };

  void OnScriptReceived(const GURL& script_url, const std::string& encoding) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << " " << script_url.spec();
    auto script_data = base::MakeUnique<
        blink::WebServiceWorkerInstalledScriptsManager::RawScriptData>();
    script_data->encoding.FromUTF8(encoding);
    auto& receivers = running_receivers_[script_url];
    script_data->script_text = receivers->body()->DrainData();
    script_data->meta_data = receivers->meta_data()->DrainData();
    parent_->FinishScriptTransfer(script_url, std::move(script_data));
    running_receivers_.erase(script_url);
  }

 private:
  class Receivers {
   public:
    Receivers(mojo::ScopedDataPipeConsumerHandle meta_data_handle,
              mojo::ScopedDataPipeConsumerHandle body_handle)
        : meta_data_(std::move(meta_data_handle)),
          body_(std::move(body_handle)) {}

    void Start(base::Closure callback) {
      callback_ = callback;
      meta_data_.Start(base::Bind(&Receivers::WaitAll, base::Unretained(this)));
      body_.Start(base::Bind(&Receivers::WaitAll, base::Unretained(this)));
    }

    Receiver* meta_data() { return &meta_data_; }
    Receiver* body() { return &body_; }

   private:
    void WaitAll() {
      if (!meta_data_.is_running() && !body_.is_running())
        callback_.Run();
    }

    base::Closure callback_;
    Receiver meta_data_;
    Receiver body_;
  };

  std::map<GURL, std::unique_ptr<Receivers>> running_receivers_;
  WebServiceWorkerInstalledScriptsManagerImpl* parent_;
  base::WeakPtrFactory<Internal> weak_factory_;
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
  TRACE_EVENT_NESTABLE_ASYNC_BEGIN0(
      "ServiceWorker", "ServiceWorkerInstalledScriptsManager::Create",
      installed_scripts_manager.get());
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  io_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&Internal::Create, installed_scripts_manager.get(),
                     std::move(installed_scripts_info->manager_request),
                     &event));
  event.Wait();
  TRACE_EVENT_NESTABLE_ASYNC_END0(
      "ServiceWorker", "ServiceWorkerInstalledScriptsManager::Create",
      installed_scripts_manager.get());
  return base::WrapUnique(installed_scripts_manager.release());
}

WebServiceWorkerInstalledScriptsManagerImpl::
    WebServiceWorkerInstalledScriptsManagerImpl(
        std::vector<GURL> installed_urls)
    : installed_urls_(std::move(installed_urls)),
      script_data_(base::MakeUnique<ScriptData>()) {}

WebServiceWorkerInstalledScriptsManagerImpl::
    ~WebServiceWorkerInstalledScriptsManagerImpl() {}

void WebServiceWorkerInstalledScriptsManagerImpl::FinishScriptTransfer(
    const GURL& script_url,
    std::unique_ptr<RawScriptData> script_data) {
  TRACE_EVENT1(
      "ServiceWorker",
      "WebServiceWorkerInstalledScriptsManagerImpl::FinishScriptTransfer",
      "script_url", script_url.spec());
  LOG(ERROR) << __PRETTY_FUNCTION__ << " " << script_url.spec();
  script_data_->Add(script_url, std::move(script_data));
}

bool WebServiceWorkerInstalledScriptsManagerImpl::IsScriptInstalled(
    const blink::WebURL& web_script_url) const {
  const GURL& script_url = web_script_url;
  LOG(ERROR) << __func__ << " check installed_scripts:"
             << "script_url: " << script_url.spec();
  for (const auto& installed_url : installed_urls_) {
    LOG(ERROR) << __func__ << " "
               << "script_url: " << script_url.spec() << " "
               << "installed_url: " << installed_url.spec();
    if (script_url == installed_url) {
      return true;
    }
  }
  LOG(ERROR) << __func__ << " script was not found";
  return false;
}

std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager::RawScriptData>
WebServiceWorkerInstalledScriptsManagerImpl::GetRawScriptData(
    const blink::WebURL& web_script_url) {
  const GURL& script_url = web_script_url;
  TRACE_EVENT1("ServiceWorker",
               "WebServiceWorkerInstalledScriptsManagerImpl::GetRawScriptData",
               "script_url", script_url.spec());
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
  return data;
}

}  // namespace content
