// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/web_service_worker_impl.h"

#include <utility>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/child/thread_safe_sender.h"
#include "content/common/service_worker/service_worker_messages.h"
#include "content/public/renderer/worker_thread.h"
#include "content/renderer/service_worker/service_worker_dispatcher.h"
#include "content/renderer/service_worker/web_service_worker_provider_impl.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"
#include "third_party/WebKit/public/platform/WebRuntimeFeatures.h"
#include "third_party/WebKit/public/platform/WebSecurityOrigin.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerProxy.h"

using blink::WebSecurityOrigin;
using blink::WebString;

namespace content {

namespace {

class ServiceWorkerHandleImpl : public blink::WebServiceWorker::Handle {
 public:
  explicit ServiceWorkerHandleImpl(scoped_refptr<WebServiceWorkerImpl> worker)
      : worker_(std::move(worker)) {}
  ~ServiceWorkerHandleImpl() override {}

  blink::WebServiceWorker* ServiceWorker() override { return worker_.get(); }

 private:
  scoped_refptr<WebServiceWorkerImpl> worker_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerHandleImpl);
};

// This lives on the IO thread with a strong binding on a Mojo connection of
// blink::mojom::ServiceWorkerObject, and forwards all received Mojo calls to
// the real impl WebServiceWorkerImpl living on a worker thread.
class WebServiceWorkerImplAdapter : public blink::mojom::ServiceWorkerObject {
 public:
  WebServiceWorkerImplAdapter(
      base::WeakPtr<WebServiceWorkerImpl> worker,
      scoped_refptr<base::SingleThreadTaskRunner> worker_task_runner)
      : worker_(worker), worker_task_runner_(std::move(worker_task_runner)) {}
  ~WebServiceWorkerImplAdapter() override = default;

 private:
  // Implements blink::mojom::ServiceWorkerObject.
  void StateChanged(blink::mojom::ServiceWorkerState state) override {
    worker_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&WebServiceWorkerImpl::StateChanged, worker_, state));
  }

  base::WeakPtr<WebServiceWorkerImpl> worker_;
  scoped_refptr<base::SingleThreadTaskRunner> worker_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(WebServiceWorkerImplAdapter);
};

void BindRequestOnIO(
    base::WeakPtr<WebServiceWorkerImpl> worker,
    scoped_refptr<base::SingleThreadTaskRunner> worker_task_runner,
    blink::mojom::ServiceWorkerObjectAssociatedRequest request) {
  // The Mojo connection of |request| may have been closed.
  if (!request.is_pending())
    return;
  mojo::MakeStrongAssociatedBinding(
      std::make_unique<WebServiceWorkerImplAdapter>(
          worker, std::move(worker_task_runner)),
      std::move(request));
}

}  // namespace

// static
scoped_refptr<WebServiceWorkerImpl>
WebServiceWorkerImpl::CreateForServiceWorkerGlobalScope(
    blink::mojom::ServiceWorkerObjectInfoPtr info,
    ThreadSafeSender* thread_safe_sender,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner) {
  DCHECK(WorkerThread::GetCurrentId());
  scoped_refptr<WebServiceWorkerImpl> impl =
      new WebServiceWorkerImpl(std::move(info), thread_safe_sender);
  impl->host_for_global_scope_ =
      blink::mojom::ThreadSafeServiceWorkerObjectHostAssociatedPtr::Create(
          std::move(impl->info_->host_ptr_info), io_task_runner);
  impl->io_task_runner_ = std::move(io_task_runner);
  impl->BindRequest(std::move(impl->info_->request));
  return impl;
}

// static
scoped_refptr<WebServiceWorkerImpl>
WebServiceWorkerImpl::CreateForServiceWorkerClient(
    blink::mojom::ServiceWorkerObjectInfoPtr info,
    ThreadSafeSender* thread_safe_sender) {
  scoped_refptr<WebServiceWorkerImpl> impl =
      new WebServiceWorkerImpl(std::move(info), thread_safe_sender);
  impl->host_for_client_.Bind(std::move(impl->info_->host_ptr_info));
  impl->BindRequest(std::move(impl->info_->request));
  return impl;
}

void WebServiceWorkerImpl::BindRequest(
    blink::mojom::ServiceWorkerObjectAssociatedRequest request) {
  // We're in a service worker execution context.
  if (WorkerThread::GetCurrentId()) {
    // Because we're trying to establish the Mojo connection for a
    // Channel-associated interface, we must bind |request| on the main or IO
    // thread.
    DCHECK(io_task_runner_);
    DCHECK(host_for_global_scope_);
    DCHECK(request.is_pending());
    io_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&BindRequestOnIO, weak_ptr_factory_.GetWeakPtr(),
                       creation_task_runner_, std::move(request)));
    return;
  }
  // We're in a service worker client context.
  binding_.Close();
  binding_.Bind(std::move(request));
}

void WebServiceWorkerImpl::StateChanged(
    blink::mojom::ServiceWorkerState state) {
  state_ = state;

  // TODO(nhiroki): This is a quick fix for http://crbug.com/507110
  DCHECK(proxy_);
  if (proxy_)
    proxy_->DispatchStateChangeEvent();
}

void WebServiceWorkerImpl::SetProxy(blink::WebServiceWorkerProxy* proxy) {
  proxy_ = proxy;
}

blink::WebServiceWorkerProxy* WebServiceWorkerImpl::Proxy() {
  return proxy_;
}

blink::WebURL WebServiceWorkerImpl::Url() const {
  return info_->url;
}

blink::mojom::ServiceWorkerState WebServiceWorkerImpl::GetState() const {
  return state_;
}

void WebServiceWorkerImpl::PostMessageToWorker(
    blink::WebServiceWorkerProvider* provider,
    const WebString& message,
    const WebSecurityOrigin& source_origin,
    blink::WebVector<blink::MessagePortChannel> channels) {
  thread_safe_sender_->Send(new ServiceWorkerHostMsg_PostMessageToWorker(
      info_->handle_id,
      static_cast<WebServiceWorkerProviderImpl*>(provider)->provider_id(),
      message.Utf16(), url::Origin(source_origin), channels.ReleaseVector()));
}

void WebServiceWorkerImpl::Terminate() {
  GetObjectHost()->Terminate();
}

// static
std::unique_ptr<blink::WebServiceWorker::Handle>
WebServiceWorkerImpl::CreateHandle(scoped_refptr<WebServiceWorkerImpl> worker) {
  if (!worker)
    return nullptr;
  return std::make_unique<ServiceWorkerHandleImpl>(std::move(worker));
}

WebServiceWorkerImpl::WebServiceWorkerImpl(
    blink::mojom::ServiceWorkerObjectInfoPtr info,
    ThreadSafeSender* thread_safe_sender)
    : binding_(this),
      creation_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      info_(std::move(info)),
      state_(info_->state),
      thread_safe_sender_(thread_safe_sender),
      proxy_(nullptr),
      weak_ptr_factory_(this) {
  DCHECK_NE(blink::mojom::kInvalidServiceWorkerHandleId, info_->handle_id);
  ServiceWorkerDispatcher* dispatcher =
      ServiceWorkerDispatcher::GetThreadSpecificInstance();
  DCHECK(dispatcher);
  dispatcher->AddServiceWorker(info_->handle_id, this);
}

WebServiceWorkerImpl::~WebServiceWorkerImpl() {
  ServiceWorkerDispatcher* dispatcher =
      ServiceWorkerDispatcher::GetThreadSpecificInstance();
  if (dispatcher)
    dispatcher->RemoveServiceWorker(info_->handle_id);
}

blink::mojom::ServiceWorkerObjectHost* WebServiceWorkerImpl::GetObjectHost() {
  if (host_for_client_)
    return host_for_client_.get();
  if (host_for_global_scope_)
    return host_for_global_scope_->get();
  NOTREACHED();
  return nullptr;
}

}  // namespace content
