// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/service_worker/web_service_worker_registration_impl.h"

#include <utility>

#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "content/child/service_worker/service_worker_dispatcher.h"
#include "content/child/service_worker/web_service_worker_impl.h"
#include "content/child/service_worker/web_service_worker_provider_impl.h"
#include "content/common/service_worker/service_worker_types.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebNavigationPreloadState.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerError.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/WebServiceWorkerRegistrationProxy.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/service_worker_registration.mojom.h"

namespace content {

namespace {

class HandleImpl : public blink::WebServiceWorkerRegistration::Handle {
 public:
  explicit HandleImpl(
      scoped_refptr<WebServiceWorkerRegistrationImpl> registration)
      : registration_(std::move(registration)) {}
  ~HandleImpl() override {}

  blink::WebServiceWorkerRegistration* Registration() override {
    return registration_.get();
  }

 private:
  scoped_refptr<WebServiceWorkerRegistrationImpl> registration_;

  DISALLOW_COPY_AND_ASSIGN(HandleImpl);
};

}  // namespace

WebServiceWorkerRegistrationImpl::QueuedTask::QueuedTask(
    QueuedTaskType type,
    const scoped_refptr<WebServiceWorkerImpl>& worker)
    : type(type), worker(worker) {}

WebServiceWorkerRegistrationImpl::QueuedTask::QueuedTask(
    const QueuedTask& other) = default;

WebServiceWorkerRegistrationImpl::QueuedTask::~QueuedTask() {}

WebServiceWorkerRegistrationImpl::WebServiceWorkerRegistrationImpl(
    blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info)
    : handle_id_(info->handle_id),
      info_(std::move(info)),
      proxy_(nullptr),
      binding_(this) {
  DCHECK(info_);
  DCHECK_NE(blink::mojom::kInvalidServiceWorkerRegistrationHandleId,
            info_->handle_id);
  DCHECK(info_->host_ptr_info.is_valid());
  DCHECK(info_->request.is_pending());

  binding_.Bind(std::move(info_->request));
  binding_.set_connection_error_handler(
      base::Bind(&WebServiceWorkerRegistrationImpl::OnConnectionError,
                 base::Unretained(this)));

  ServiceWorkerDispatcher* dispatcher =
      ServiceWorkerDispatcher::GetThreadSpecificInstance();
  DCHECK(dispatcher);
  dispatcher->AddServiceWorkerRegistration(handle_id_, this);
}

void WebServiceWorkerRegistrationImpl::SetInstalling(
    const scoped_refptr<WebServiceWorkerImpl>& service_worker) {
  if (!info_)
    return;
  if (proxy_)
    proxy_->SetInstalling(WebServiceWorkerImpl::CreateHandle(service_worker));
  else
    queued_tasks_.push_back(QueuedTask(INSTALLING, service_worker));
}

void WebServiceWorkerRegistrationImpl::SetWaiting(
    const scoped_refptr<WebServiceWorkerImpl>& service_worker) {
  if (!info_)
    return;
  if (proxy_)
    proxy_->SetWaiting(WebServiceWorkerImpl::CreateHandle(service_worker));
  else
    queued_tasks_.push_back(QueuedTask(WAITING, service_worker));
}

void WebServiceWorkerRegistrationImpl::SetActive(
    const scoped_refptr<WebServiceWorkerImpl>& service_worker) {
  if (!info_)
    return;
  if (proxy_)
    proxy_->SetActive(WebServiceWorkerImpl::CreateHandle(service_worker));
  else
    queued_tasks_.push_back(QueuedTask(ACTIVE, service_worker));
}

void WebServiceWorkerRegistrationImpl::OnUpdateFound() {
  if (!info_)
    return;
  if (proxy_)
    proxy_->DispatchUpdateFoundEvent();
  else
    queued_tasks_.push_back(QueuedTask(UPDATE_FOUND, nullptr));
}

void WebServiceWorkerRegistrationImpl::SetProxy(
    blink::WebServiceWorkerRegistrationProxy* proxy) {
  DCHECK(info_);
  proxy_ = proxy;
  RunQueuedTasks();
}

void WebServiceWorkerRegistrationImpl::RunQueuedTasks() {
  DCHECK(proxy_);
  for (const QueuedTask& task : queued_tasks_) {
    if (task.type == INSTALLING)
      proxy_->SetInstalling(WebServiceWorkerImpl::CreateHandle(task.worker));
    else if (task.type == WAITING)
      proxy_->SetWaiting(WebServiceWorkerImpl::CreateHandle(task.worker));
    else if (task.type == ACTIVE)
      proxy_->SetActive(WebServiceWorkerImpl::CreateHandle(task.worker));
    else if (task.type == UPDATE_FOUND)
      proxy_->DispatchUpdateFoundEvent();
  }
  queued_tasks_.clear();
}

void WebServiceWorkerRegistrationImpl::Attach(
    blink::mojom::ServiceWorkerRegistrationObjectInfoPtr info) {
  if (info_)
    return;
  DCHECK(info);
  DCHECK_NE(blink::mojom::kInvalidServiceWorkerRegistrationHandleId,
            info->handle_id);
  DCHECK_EQ(handle_id_, info->handle_id);
  DCHECK(info->host_ptr_info.is_valid());
  DCHECK(!info->request.is_pending());
  info_ = std::move(info);
}

void WebServiceWorkerRegistrationImpl::DetachAndMaybeDestroy() {
  proxy_ = nullptr;
  queued_tasks_.clear();
  // This will close the Mojo connection of interface
  // ServiceWorkerRegistrationObjectHost to the remote
  // content::ServiceWorkerRegistrationHandle instance in the browser process,
  // which will get destroyed if all Mojo connections to it have been broken.
  // Then, destruction of content::ServiceWorkerRegistrationHandle will close
  // the Mojo connection of interface ServiceWorkerRegistrationObject to |this|,
  // which will then get destroyed by the connection error handler
  // OnConnectionError().
  info_ = nullptr;
}

void WebServiceWorkerRegistrationImpl::OnConnectionError() {
  delete this;
}

blink::WebServiceWorkerRegistrationProxy*
WebServiceWorkerRegistrationImpl::Proxy() {
  return proxy_;
}

blink::WebURL WebServiceWorkerRegistrationImpl::Scope() const {
  return info_->options->scope;
}

void WebServiceWorkerRegistrationImpl::Update(
    blink::WebServiceWorkerProvider* provider,
    std::unique_ptr<WebServiceWorkerUpdateCallbacks> callbacks) {
  DCHECK(info_->host_ptr_info.is_valid());
  WebServiceWorkerProviderImpl* provider_impl =
      static_cast<WebServiceWorkerProviderImpl*>(provider);
  ServiceWorkerDispatcher* dispatcher =
      ServiceWorkerDispatcher::GetThreadSpecificInstance();
  DCHECK(dispatcher);
  dispatcher->UpdateServiceWorker(provider_impl->provider_id(),
                                  RegistrationId(), std::move(callbacks));
}

void WebServiceWorkerRegistrationImpl::Unregister(
    blink::WebServiceWorkerProvider* provider,
    std::unique_ptr<WebServiceWorkerUnregistrationCallbacks> callbacks) {
  DCHECK(info_->host_ptr_info.is_valid());
  WebServiceWorkerProviderImpl* provider_impl =
      static_cast<WebServiceWorkerProviderImpl*>(provider);
  ServiceWorkerDispatcher* dispatcher =
      ServiceWorkerDispatcher::GetThreadSpecificInstance();
  DCHECK(dispatcher);
  dispatcher->UnregisterServiceWorker(provider_impl->provider_id(),
                                      RegistrationId(), std::move(callbacks));
}

void WebServiceWorkerRegistrationImpl::EnableNavigationPreload(
    bool enable,
    blink::WebServiceWorkerProvider* provider,
    std::unique_ptr<WebEnableNavigationPreloadCallbacks> callbacks) {
  DCHECK(info_->host_ptr_info.is_valid());
  WebServiceWorkerProviderImpl* provider_impl =
      static_cast<WebServiceWorkerProviderImpl*>(provider);
  ServiceWorkerDispatcher* dispatcher =
      ServiceWorkerDispatcher::GetThreadSpecificInstance();
  DCHECK(dispatcher);
  dispatcher->EnableNavigationPreload(provider_impl->provider_id(),
                                      RegistrationId(), enable,
                                      std::move(callbacks));
}

void WebServiceWorkerRegistrationImpl::GetNavigationPreloadState(
    blink::WebServiceWorkerProvider* provider,
    std::unique_ptr<WebGetNavigationPreloadStateCallbacks> callbacks) {
  DCHECK(info_->host_ptr_info.is_valid());
  WebServiceWorkerProviderImpl* provider_impl =
      static_cast<WebServiceWorkerProviderImpl*>(provider);
  ServiceWorkerDispatcher* dispatcher =
      ServiceWorkerDispatcher::GetThreadSpecificInstance();
  DCHECK(dispatcher);
  dispatcher->GetNavigationPreloadState(provider_impl->provider_id(),
                                        RegistrationId(), std::move(callbacks));
}

void WebServiceWorkerRegistrationImpl::SetNavigationPreloadHeader(
    const blink::WebString& value,
    blink::WebServiceWorkerProvider* provider,
    std::unique_ptr<WebSetNavigationPreloadHeaderCallbacks> callbacks) {
  DCHECK(info_->host_ptr_info.is_valid());
  WebServiceWorkerProviderImpl* provider_impl =
      static_cast<WebServiceWorkerProviderImpl*>(provider);
  ServiceWorkerDispatcher* dispatcher =
      ServiceWorkerDispatcher::GetThreadSpecificInstance();
  DCHECK(dispatcher);
  dispatcher->SetNavigationPreloadHeader(provider_impl->provider_id(),
                                         RegistrationId(), value.Utf8(),
                                         std::move(callbacks));
}

int64_t WebServiceWorkerRegistrationImpl::RegistrationId() const {
  return info_->registration_id;
}

// static
std::unique_ptr<blink::WebServiceWorkerRegistration::Handle>
WebServiceWorkerRegistrationImpl::CreateHandle(
    scoped_refptr<WebServiceWorkerRegistrationImpl> registration) {
  if (!registration)
    return nullptr;
  return std::make_unique<HandleImpl>(std::move(registration));
}

// static
void WebServiceWorkerRegistrationImpl::Destruct(
    const WebServiceWorkerRegistrationImpl* x) {
  const_cast<WebServiceWorkerRegistrationImpl*>(x)->DetachAndMaybeDestroy();
}

WebServiceWorkerRegistrationImpl::~WebServiceWorkerRegistrationImpl() {
  ServiceWorkerDispatcher* dispatcher =
      ServiceWorkerDispatcher::GetThreadSpecificInstance();
  if (dispatcher)
    dispatcher->RemoveServiceWorkerRegistration(handle_id_);
}

}  // namespace content
