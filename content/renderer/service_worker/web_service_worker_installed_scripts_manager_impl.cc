// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/web_service_worker_installed_scripts_manager_impl.h"

#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
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

  blink::WebVector<blink::WebVector<char>> TakeData() {
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
  static void Create(
      scoped_refptr<WebServiceWorkerInstalledScriptsManagerImpl::
                        ThreadSafeScriptContainer> script_container,
      mojom::ServiceWorkerInstalledScriptsManagerRequest request) {
    TRACE_EVENT0("ServiceWorker",
                 "ServiceWorkerInstalledScriptsManager::Internal");
    mojo::MakeStrongBinding(
        base::MakeUnique<Internal>(std::move(script_container)),
        std::move(request));
  }

  Internal(scoped_refptr<WebServiceWorkerInstalledScriptsManagerImpl::
                             ThreadSafeScriptContainer> script_container)
      : script_container_(script_container), weak_factory_(this) {}

  ~Internal() override { script_container_->OnCompleted(); }

  // Implements mojom::ServiceWorkerInstalledScriptsManager.
  // Called on the io thread.
  void TransferInstalledScript(
      mojom::ServiceWorkerScriptInfoPtr script_info) override {
    const GURL& script_url = script_info->script_url;
    auto receivers = base::MakeUnique<Receivers>(
        std::move(script_info->meta_data), std::move(script_info->body));
    receivers->Start(base::Bind(&Internal::OnScriptReceived,
                                weak_factory_.GetWeakPtr(),
                                base::Passed(&script_info)));
    running_receivers_[script_url] = std::move(receivers);
  }

  void OnScriptReceived(mojom::ServiceWorkerScriptInfoPtr script_info) {
    const GURL& script_url = script_info->script_url;
    auto& receivers = running_receivers_[script_url];
    auto script_data = base::MakeUnique<
        blink::WebServiceWorkerInstalledScriptsManager::RawScriptData>(
        blink::WebString::FromUTF8(script_info->encoding),
        receivers->body()->TakeData(), receivers->meta_data()->TakeData());
    for (const auto& entry : script_info->headers) {
      script_data->AddHeader(blink::WebString::FromUTF8(entry.first),
                             blink::WebString::FromUTF8(entry.second));
    }
    script_container_->Add(script_url, std::move(script_data));
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
  scoped_refptr<
      WebServiceWorkerInstalledScriptsManagerImpl::ThreadSafeScriptContainer>
      script_container_;
  base::WeakPtrFactory<Internal> weak_factory_;
};

}  // namespace

WebServiceWorkerInstalledScriptsManagerImpl::ThreadSafeScriptContainer::
    ThreadSafeScriptContainer()
    : lock_(), waiting_cv_(&lock_), is_completed(false) {}

void WebServiceWorkerInstalledScriptsManagerImpl::ThreadSafeScriptContainer::
    Add(const GURL& url, std::unique_ptr<RawScriptData> data) {
  base::AutoLock lock(lock_);
  script_data_.emplace(url, std::move(data));
  if (url == waiting_url_)
    waiting_cv_.Signal();
}

bool WebServiceWorkerInstalledScriptsManagerImpl::ThreadSafeScriptContainer::
    Exists(const GURL& url) {
  base::AutoLock lock(lock_);
  return script_data_.find(url) != script_data_.end();
}

bool WebServiceWorkerInstalledScriptsManagerImpl::ThreadSafeScriptContainer::
    Wait(const GURL& url) {
  base::AutoLock lock(lock_);
  waiting_url_ = url;
  while (script_data_.find(url) == script_data_.end()) {
    if (is_completed)
      return false;
    waiting_cv_.Wait();
  }
  waiting_url_ = GURL();
  return true;
}

std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager::RawScriptData>
WebServiceWorkerInstalledScriptsManagerImpl::ThreadSafeScriptContainer::Take(
    const GURL& url) {
  base::AutoLock lock(lock_);
  DCHECK(script_data_.find(url) != script_data_.end());
  auto data = std::move(script_data_[url]);
  DCHECK(script_data_[url] == nullptr);
  return data;
}

void WebServiceWorkerInstalledScriptsManagerImpl::ThreadSafeScriptContainer::
    OnCompleted() {
  base::AutoLock lock(lock_);
  is_completed = true;
  waiting_cv_.Broadcast();
}

WebServiceWorkerInstalledScriptsManagerImpl::ThreadSafeScriptContainer::
    ~ThreadSafeScriptContainer() = default;

// static
std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager>
WebServiceWorkerInstalledScriptsManagerImpl::Create(
    mojom::ServiceWorkerInstalledScriptsInfoPtr installed_scripts_info,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner) {
  TRACE_EVENT0("ServiceWorker",
               "WebServiceWorkerInstalledScriptsManager::Create");
  auto script_container = base::MakeRefCounted<ThreadSafeScriptContainer>();
  auto installed_scripts_manager =
      base::WrapUnique<WebServiceWorkerInstalledScriptsManagerImpl>(
          new WebServiceWorkerInstalledScriptsManagerImpl(
              std::move(installed_scripts_info->installed_urls),
              script_container));
  io_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&Internal::Create, script_container,
                     std::move(installed_scripts_info->manager_request)));
  return base::WrapUnique(installed_scripts_manager.release());
}

WebServiceWorkerInstalledScriptsManagerImpl::
    WebServiceWorkerInstalledScriptsManagerImpl(
        std::vector<GURL>&& installed_urls,
        scoped_refptr<ThreadSafeScriptContainer> script_container)
    : installed_urls_(installed_urls.begin(), installed_urls.end()),
      script_container_(std::move(script_container)) {}

WebServiceWorkerInstalledScriptsManagerImpl::
    ~WebServiceWorkerInstalledScriptsManagerImpl() = default;

void WebServiceWorkerInstalledScriptsManagerImpl::FinishScriptTransfer(
    const GURL& script_url,
    std::unique_ptr<RawScriptData> script_data) {
  TRACE_EVENT1(
      "ServiceWorker",
      "WebServiceWorkerInstalledScriptsManagerImpl::FinishScriptTransfer",
      "script_url", script_url.spec());
  script_container_->Add(script_url, std::move(script_data));
}

bool WebServiceWorkerInstalledScriptsManagerImpl::IsScriptInstalled(
    const blink::WebURL& web_script_url) const {
  return base::ContainsKey(installed_urls_, web_script_url);
}

std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager::RawScriptData>
WebServiceWorkerInstalledScriptsManagerImpl::GetRawScriptData(
    const blink::WebURL& web_script_url) {
  const GURL& script_url = web_script_url;
  TRACE_EVENT1("ServiceWorker",
               "WebServiceWorkerInstalledScriptsManagerImpl::GetRawScriptData",
               "script_url", script_url.spec());
  if (!IsScriptInstalled(web_script_url)) {
    return nullptr;
  }

  if (!script_container_->Exists(script_url)) {
    // Wait for arrival of the script.
    const bool success = script_container_->Wait(script_url);
    if (!success)
      return nullptr;
    DCHECK(script_container_->Exists(script_url));
  }
  auto data = script_container_->Take(script_url);
  // It's possible to be null when the data has already been taken.
  if (!data) {
    // TODO(shimazu): Ask to the browser process when the script has already
    // been served
    return nullptr;
  }
  return data;
}

}  // namespace content
