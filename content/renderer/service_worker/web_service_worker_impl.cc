// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/service_worker/web_service_worker_impl.h"

#include <utility>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "content/child/thread_safe_sender.h"
#include "content/common/service_worker/service_worker_messages.h"
#include "content/renderer/service_worker/service_worker_dispatcher.h"
#include "content/renderer/service_worker/web_service_worker_provider_impl.h"
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
  explicit ServiceWorkerHandleImpl(
      const scoped_refptr<WebServiceWorkerImpl>& worker)
      : worker_(worker) {}
  ~ServiceWorkerHandleImpl() override {}

  blink::WebServiceWorker* ServiceWorker() override { return worker_.get(); }

 private:
  scoped_refptr<WebServiceWorkerImpl> worker_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerHandleImpl);
};

}  // namespace

WebServiceWorkerImpl::WebServiceWorkerImpl(
    blink::mojom::ServiceWorkerObjectInfoPtr info,
    ThreadSafeSender* thread_safe_sender)
    : info_(std::move(info)),
      state_(info_->state),
      thread_safe_sender_(thread_safe_sender),
      proxy_(nullptr) {
  DCHECK_NE(blink::mojom::kInvalidServiceWorkerHandleId, info_->handle_id);
  ServiceWorkerDispatcher* dispatcher =
      ServiceWorkerDispatcher::GetThreadSpecificInstance();
  DCHECK(dispatcher);
  dispatcher->AddServiceWorker(info_->handle_id, this);
}

void WebServiceWorkerImpl::OnStateChanged(
    blink::mojom::ServiceWorkerState new_state) {
  state_ = new_state;

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
    blink::TransferableMessage message,
    const WebSecurityOrigin& source_origin) {
  thread_safe_sender_->Send(new ServiceWorkerHostMsg_PostMessageToWorker(
      info_->handle_id,
      static_cast<WebServiceWorkerProviderImpl*>(provider)->provider_id(),
      new base::RefCountedData<blink::TransferableMessage>(std::move(message)),
      url::Origin(source_origin)));
}

void WebServiceWorkerImpl::Terminate() {
  thread_safe_sender_->Send(
      new ServiceWorkerHostMsg_TerminateWorker(info_->handle_id));
}

// static
std::unique_ptr<blink::WebServiceWorker::Handle>
WebServiceWorkerImpl::CreateHandle(
    const scoped_refptr<WebServiceWorkerImpl>& worker) {
  if (!worker)
    return nullptr;
  return std::make_unique<ServiceWorkerHandleImpl>(worker);
}

WebServiceWorkerImpl::~WebServiceWorkerImpl() {
  ServiceWorkerDispatcher* dispatcher =
      ServiceWorkerDispatcher::GetThreadSpecificInstance();
  if (dispatcher)
    dispatcher->RemoveServiceWorker(info_->handle_id);
}

}  // namespace content
