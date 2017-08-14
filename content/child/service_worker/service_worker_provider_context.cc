// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/service_worker/service_worker_provider_context.h"

#include <utility>

#include "base/macros.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/child/child_thread_impl.h"
#include "content/child/service_worker/service_worker_dispatcher.h"
#include "content/child/service_worker/service_worker_handle_reference.h"
#include "content/child/service_worker/service_worker_registration_handle_reference.h"
#include "content/child/service_worker/web_service_worker_impl.h"
#include "content/child/thread_safe_sender.h"
#include "content/child/webmessageportchannel_impl.h"
#include "content/child/worker_thread_registry.h"

namespace content {

class ServiceWorkerProviderContext::Delegate {
 public:
  virtual ~Delegate() {}
  virtual void AssociateRegistration(
      std::unique_ptr<ServiceWorkerRegistrationHandleReference> registration,
      std::unique_ptr<ServiceWorkerHandleReference> installing,
      std::unique_ptr<ServiceWorkerHandleReference> waiting,
      std::unique_ptr<ServiceWorkerHandleReference> active) = 0;
  virtual void DisassociateRegistration() = 0;
  virtual void GetAssociatedRegistration(
      ServiceWorkerRegistrationObjectInfo* info,
      ServiceWorkerVersionAttributes* attrs) = 0;
  virtual bool HasAssociatedRegistration() = 0;
  virtual void SetController(
      std::unique_ptr<ServiceWorkerHandleReference> controller) = 0;
  virtual ServiceWorkerHandleReference* controller() = 0;
};

// Delegate class for ServiceWorker client (Document, SharedWorker, etc) to
// keep the associated registration and the controller until
// ServiceWorkerContainer is initialized.
class ServiceWorkerProviderContext::ControlleeDelegate
    : public ServiceWorkerProviderContext::Delegate {
 public:
  ControlleeDelegate() {}
  ~ControlleeDelegate() override {}

  void AssociateRegistration(
      std::unique_ptr<ServiceWorkerRegistrationHandleReference> registration,
      std::unique_ptr<ServiceWorkerHandleReference> /* installing */,
      std::unique_ptr<ServiceWorkerHandleReference> /* waiting */,
      std::unique_ptr<ServiceWorkerHandleReference> /* active */) override {
    DCHECK(!registration_);
    registration_ = std::move(registration);
  }

  void DisassociateRegistration() override {
    controller_.reset();
    registration_.reset();
  }

  void SetController(
      std::unique_ptr<ServiceWorkerHandleReference> controller) override {
    DCHECK(registration_);
    DCHECK(!controller ||
           controller->handle_id() != kInvalidServiceWorkerHandleId);
    controller_ = std::move(controller);
  }

  bool HasAssociatedRegistration() override { return !!registration_; }

  void GetAssociatedRegistration(
      ServiceWorkerRegistrationObjectInfo* /* info */,
      ServiceWorkerVersionAttributes* /* attrs */) override {
    NOTREACHED();
  }

  ServiceWorkerHandleReference* controller() override {
    return controller_.get();
  }

 private:
  std::unique_ptr<ServiceWorkerRegistrationHandleReference> registration_;
  std::unique_ptr<ServiceWorkerHandleReference> controller_;

  DISALLOW_COPY_AND_ASSIGN(ControlleeDelegate);
};

// Delegate class for ServiceWorkerGlobalScope to keep the associated
// registration and its versions until the execution context is initialized.
class ServiceWorkerProviderContext::ControllerDelegate
    : public ServiceWorkerProviderContext::Delegate {
 public:
  ControllerDelegate() {}
  ~ControllerDelegate() override {}

  void AssociateRegistration(
      std::unique_ptr<ServiceWorkerRegistrationHandleReference> registration,
      std::unique_ptr<ServiceWorkerHandleReference> installing,
      std::unique_ptr<ServiceWorkerHandleReference> waiting,
      std::unique_ptr<ServiceWorkerHandleReference> active) override {
    DCHECK(!registration_);
    registration_ = std::move(registration);
    installing_ = std::move(installing);
    waiting_ = std::move(waiting);
    active_ = std::move(active);
  }

  void DisassociateRegistration() override {
    // ServiceWorkerGlobalScope is never disassociated.
    NOTREACHED();
  }

  void SetController(
      std::unique_ptr<ServiceWorkerHandleReference> /* controller */) override {
    NOTREACHED();
  }

  bool HasAssociatedRegistration() override { return !!registration_; }

  void GetAssociatedRegistration(
      ServiceWorkerRegistrationObjectInfo* info,
      ServiceWorkerVersionAttributes* attrs) override {
    DCHECK(HasAssociatedRegistration());
    *info = registration_->info();
    if (installing_)
      attrs->installing = installing_->info();
    if (waiting_)
      attrs->waiting = waiting_->info();
    if (active_)
      attrs->active = active_->info();
  }

  ServiceWorkerHandleReference* controller() override {
    NOTREACHED();
    return nullptr;
  }

 private:
  std::unique_ptr<ServiceWorkerRegistrationHandleReference> registration_;
  std::unique_ptr<ServiceWorkerHandleReference> installing_;
  std::unique_ptr<ServiceWorkerHandleReference> waiting_;
  std::unique_ptr<ServiceWorkerHandleReference> active_;

  DISALLOW_COPY_AND_ASSIGN(ControllerDelegate);
};

ServiceWorkerProviderContext::ServiceWorkerProviderContext(
    int provider_id,
    ServiceWorkerProviderType provider_type,
    mojom::ServiceWorkerProviderAssociatedRequest request,
    ThreadSafeSender* thread_safe_sender)
    : provider_id_(provider_id),
      main_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      thread_safe_sender_(thread_safe_sender),
      binding_(this, std::move(request)) {
  if (provider_type == SERVICE_WORKER_PROVIDER_FOR_CONTROLLER)
    delegate_.reset(new ControllerDelegate);
  else
    delegate_.reset(new ControlleeDelegate);

  ServiceWorkerDispatcher* dispatcher =
      ServiceWorkerDispatcher::GetOrCreateThreadSpecificInstance(
          thread_safe_sender_.get(), main_thread_task_runner_.get());
  dispatcher->AddProviderContext(this);
}

ServiceWorkerProviderContext::~ServiceWorkerProviderContext() {
  if (ServiceWorkerDispatcher* dispatcher =
          ServiceWorkerDispatcher::GetThreadSpecificInstance()) {
    // Remove this context from the dispatcher living on the main thread.
    dispatcher->RemoveProviderContext(this);
  }
}

void ServiceWorkerProviderContext::AssociateRegistration(
    const ServiceWorkerRegistrationObjectInfo& info,
    const ServiceWorkerVersionAttributes& attrs) {
  DCHECK(main_thread_task_runner_->RunsTasksInCurrentSequence());
  // Adopt the references sent from the browser process and pass them to the
  // provider context if it exists.
  std::unique_ptr<ServiceWorkerRegistrationHandleReference> registration =
      Adopt(info);
  std::unique_ptr<ServiceWorkerHandleReference> installing =
      Adopt(attrs.installing);
  std::unique_ptr<ServiceWorkerHandleReference> waiting = Adopt(attrs.waiting);
  std::unique_ptr<ServiceWorkerHandleReference> active = Adopt(attrs.active);

  delegate_->AssociateRegistration(std::move(registration),
                                   std::move(installing), std::move(waiting),
                                   std::move(active));
}

void ServiceWorkerProviderContext::DisassociateRegistration() {
  DCHECK(main_thread_task_runner_->RunsTasksInCurrentSequence());
  delegate_->DisassociateRegistration();
}

void ServiceWorkerProviderContext::SetControllerServiceWorker(
    const ServiceWorkerObjectInfo& info,
    const std::vector<uint32_t>& used_features,
    bool should_notify_controllerchange) {
  DCHECK(main_thread_task_runner_->RunsTasksInCurrentSequence());
  std::unique_ptr<ServiceWorkerHandleReference> controller = Adopt(info);
  delegate_->SetController(std::move(controller));
  std::set<uint32_t> temp(used_features.begin(), used_features.end());
  used_features_ = temp;
  for (uint32_t feature : used_features_)
    provider_client_->CountFeature(feature);

  ServiceWorkerDispatcher* dispatcher =
      ServiceWorkerDispatcher::GetThreadSpecificInstance();
  scoped_refptr<WebServiceWorkerImpl> worker =
      dispatcher->GetOrCreateServiceWorker(ServiceWorkerHandleReference::Create(
          info, thread_safe_sender_.get()));
  provider_client_->SetController(WebServiceWorkerImpl::CreateHandle(worker),
                                  should_notify_controllerchange);
}

void ServiceWorkerProviderContext::MessageToDocument(
    mojom::MessageToDocumentParamsPtr params) {
  // Make sure we're on the main document thread. (That must be the only
  // thread we get this message)
  DCHECK_EQ(kDocumentMainThreadId, params->thread_id);

  // Adopt the reference sent from the browser process and get the corresponding
  // worker object.
  ServiceWorkerDispatcher* dispatcher =
      ServiceWorkerDispatcher::GetThreadSpecificInstance();
  scoped_refptr<WebServiceWorkerImpl> worker =
      dispatcher->GetOrCreateServiceWorker(Adopt(params->service_worker_info));

  blink::WebMessagePortChannelArray ports =
      WebMessagePortChannelImpl::CreateFromMessagePipeHandles(
          std::move(params->message_ports));

  provider_client_->DispatchMessageEvent(
      WebServiceWorkerImpl::CreateHandle(worker),
      blink::WebString::FromUTF16(params->message), std::move(ports));
}

void ServiceWorkerProviderContext::GetAssociatedRegistration(
    ServiceWorkerRegistrationObjectInfo* info,
    ServiceWorkerVersionAttributes* attrs) {
  DCHECK(!main_thread_task_runner_->RunsTasksInCurrentSequence());
  delegate_->GetAssociatedRegistration(info, attrs);
}

bool ServiceWorkerProviderContext::HasAssociatedRegistration() {
  return delegate_->HasAssociatedRegistration();
}

ServiceWorkerHandleReference* ServiceWorkerProviderContext::controller() {
  DCHECK(main_thread_task_runner_->RunsTasksInCurrentSequence());
  return delegate_->controller();
}

void ServiceWorkerProviderContext::CountFeature(uint32_t feature) {
  // ServiceWorkerProviderContext keeps track of features in order to propagate
  // it to WebServiceWorkerProviderClient, which actually records the
  // UseCounter.
  used_features_.insert(feature);
}

void ServiceWorkerProviderContext::DestructOnMainThread() const {
  if (!main_thread_task_runner_->RunsTasksInCurrentSequence() &&
      main_thread_task_runner_->DeleteSoon(FROM_HERE, this)) {
    return;
  }
  delete this;
}
std::unique_ptr<ServiceWorkerRegistrationHandleReference>
ServiceWorkerProviderContext::Adopt(
    const ServiceWorkerRegistrationObjectInfo& info) {
  return ServiceWorkerRegistrationHandleReference::Adopt(
      info, thread_safe_sender_.get());
}

std::unique_ptr<ServiceWorkerHandleReference>
ServiceWorkerProviderContext::Adopt(const ServiceWorkerObjectInfo& info) {
  return ServiceWorkerHandleReference::Adopt(info, thread_safe_sender_.get());
}

}  // namespace content
