// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/web_service_worker_installed_scripts_manager_impl.h"

#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

namespace {

// Receiver is a class to read a Mojo data pipe. Received data are stored in
// chunks. Lives on the IO thread.
class Receiver {
 public:
  using BytesChunk = blink::WebVector<char>;

  Receiver(mojo::ScopedDataPipeConsumerHandle handle)
      : handle_(std::move(handle)) {}

  void Start(base::Closure callback) {
    // TODO(shimazu): Receive data from the handle.
    callback.Run();
  }

  bool is_running() const { return handle_.is_valid(); }

  blink::WebVector<BytesChunk> TakeChunks() {
    return blink::WebVector<blink::WebVector<char>>(std::move(chunks_));
  }

 private:
  mojo::ScopedDataPipeConsumerHandle handle_;

  // std::vector is internally used because blink::WebVector doesn't have
  // push_back.
  std::vector<BytesChunk> chunks_;
};

// Receivers is a helper class to wait for the end of reading body and meta
// data. Lives on the IO thread.
class Receivers {
 public:
  Receivers(mojo::ScopedDataPipeConsumerHandle meta_data_handle,
            mojo::ScopedDataPipeConsumerHandle body_handle)
      : meta_data_(std::move(meta_data_handle)),
        body_(std::move(body_handle)) {}

  // Starts reading the pipes and invokes |callback| when both is finished.
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

// Internal lives on the IO thread. This receives mojom::ServiceWorkerScriptInfo
// for all installed scripts and then starts reading the body and meta data from
// the browser. This instance will be kept alive as long as the Mojo's
// connection is established.
class Internal : public mojom::ServiceWorkerInstalledScriptsManager {
 public:
  // Called on the IO thread.
  // Creates and binds a new Internal instance to |request|.
  static void Create(
      scoped_refptr<WebServiceWorkerInstalledScriptsManagerImpl::
                        ThreadSafeScriptContainer> script_container,
      mojom::ServiceWorkerInstalledScriptsManagerRequest request) {
    mojo::MakeStrongBinding(
        base::MakeUnique<Internal>(std::move(script_container)),
        std::move(request));
  }

  Internal(scoped_refptr<WebServiceWorkerInstalledScriptsManagerImpl::
                             ThreadSafeScriptContainer> script_container)
      : script_container_(script_container), weak_factory_(this) {}

  ~Internal() override {
    // Wake up a waiting thread not to wait forever. If the script has not been
    // reached until here, that means something went wrong.
    script_container_->OnCompleted();
  }

  // Implements mojom::ServiceWorkerInstalledScriptsManager.
  // Called on the IO thread.
  void TransferInstalledScript(
      mojom::ServiceWorkerScriptInfoPtr script_info) override {
    GURL script_url = script_info->script_url;
    auto receivers = base::MakeUnique<Receivers>(
        std::move(script_info->meta_data), std::move(script_info->body));
    receivers->Start(base::Bind(&Internal::OnScriptReceived,
                                weak_factory_.GetWeakPtr(),
                                base::Passed(&script_info)));
    running_receivers_[script_url] = std::move(receivers);
  }

  // Called on the IO thread.
  void OnScriptReceived(mojom::ServiceWorkerScriptInfoPtr script_info) {
    const GURL& script_url = script_info->script_url;
    Receivers* receivers = running_receivers_[script_url].get();
    DCHECK(receivers);
    auto script_data = base::MakeUnique<
        blink::WebServiceWorkerInstalledScriptsManager::RawScriptData>(
        blink::WebString::FromUTF8(script_info->encoding),
        receivers->body()->TakeChunks(), receivers->meta_data()->TakeChunks());
    for (const auto& entry : script_info->headers) {
      script_data->AddHeader(blink::WebString::FromUTF8(entry.first),
                             blink::WebString::FromUTF8(entry.second));
    }
    script_container_->Add(script_url, std::move(script_data));
    running_receivers_.erase(script_url);
  }

 private:
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
  script_data_[url] = std::move(data);
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
  DCHECK(waiting_url_.is_empty()) << "Only one thread can wait at a time.";
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
  DCHECK(script_data_.find(url) != script_data_.end())
      << "Script should be added before calling Take.";
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
  return installed_scripts_manager;
}

WebServiceWorkerInstalledScriptsManagerImpl::
    WebServiceWorkerInstalledScriptsManagerImpl(
        std::vector<GURL>&& installed_urls,
        scoped_refptr<ThreadSafeScriptContainer> script_container)
    : installed_urls_(installed_urls.begin(), installed_urls.end()),
      script_container_(std::move(script_container)) {}

WebServiceWorkerInstalledScriptsManagerImpl::
    ~WebServiceWorkerInstalledScriptsManagerImpl() = default;

bool WebServiceWorkerInstalledScriptsManagerImpl::IsScriptInstalled(
    const blink::WebURL& script_url) const {
  return base::ContainsKey(installed_urls_, script_url);
}

std::unique_ptr<blink::WebServiceWorkerInstalledScriptsManager::RawScriptData>
WebServiceWorkerInstalledScriptsManagerImpl::GetRawScriptData(
    const blink::WebURL& script_url) {
  if (!IsScriptInstalled(script_url))
    return nullptr;

  if (!script_container_->Exists(script_url)) {
    // Wait for arrival of the script.
    const bool success = script_container_->Wait(script_url);
    // It can fail due to an error on Mojo pipes.
    if (!success)
      return nullptr;
    DCHECK(script_container_->Exists(script_url));
  }
  auto data = script_container_->Take(script_url);
  // It's possible to be null when the data has already been taken.
  if (!data) {
    // TODO(shimazu): Ask the browser process when the script has already been
    // served.
    return nullptr;
  }
  return data;
}

}  // namespace content
