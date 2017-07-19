// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/payments/payment_app_provider_impl.h"

#include "content/browser/payments/payment_app_context_impl.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/browser/service_worker/service_worker_metrics.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "content/browser/storage_partition_impl.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "content/common/service_worker/service_worker_utils.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/common/time.mojom.h"

namespace content {
namespace {

class InvokeCallback : public payments::mojom::PaymentHandlerInvokeCallback {
 public:
  static payments::mojom::PaymentHandlerInvokeCallbackPtr Create(
      int payment_request_id,
      scoped_refptr<ServiceWorkerVersion> service_worker_version,
      const PaymentAppProvider::InvokePaymentAppCallback callback,
      const PaymentAppProvider::InvokePaymentAppStatusCallback
          status_callback) {
    InvokeCallback* invoke_callback = new InvokeCallback(
        payment_request_id, std::move(service_worker_version), callback,
        status_callback);
    payments::mojom::PaymentHandlerInvokeCallbackPtr callback_proxy;
    invoke_callback->binding_.Bind(mojo::MakeRequest(&callback_proxy));
    return callback_proxy;
  }
  ~InvokeCallback() override {}

  void OnPaymentHandlerResponse(
      payments::mojom::PaymentHandlerResponsePtr response,
      base::Time dispatch_event_time) override {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    service_worker_version_->FinishRequest(payment_request_id_, false,
                                           std::move(dispatch_event_time));
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(callback_, base::Passed(std::move(response))));
    delete this;
  }

  void IsCancelled(IsCancelledCallback callback) override {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);

    BrowserThread::PostTaskAndReplyWithResult<bool, bool>(
        BrowserThread::UI, FROM_HERE, status_callback_, std::move(callback));
  }

 private:
  InvokeCallback(
      int payment_request_id,
      scoped_refptr<ServiceWorkerVersion> service_worker_version,
      const PaymentAppProvider::InvokePaymentAppCallback callback,
      const PaymentAppProvider::InvokePaymentAppStatusCallback status_callback)
      : payment_request_id_(payment_request_id),
        service_worker_version_(service_worker_version),
        callback_(callback),
        status_callback_(status_callback),
        binding_(this) {}

  int payment_request_id_;
  scoped_refptr<ServiceWorkerVersion> service_worker_version_;
  const PaymentAppProvider::InvokePaymentAppCallback callback_;
  const PaymentAppProvider::InvokePaymentAppStatusCallback status_callback_;
  mojo::Binding<payments::mojom::PaymentHandlerInvokeCallback> binding_;
};

void DidGetAllPaymentAppsOnIO(
    PaymentAppProvider::GetAllPaymentAppsCallback callback,
    PaymentAppProvider::PaymentApps apps) {
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(std::move(callback), base::Passed(std::move(apps))));
}

void GetAllPaymentAppsOnIO(
    scoped_refptr<PaymentAppContextImpl> payment_app_context,
    PaymentAppProvider::GetAllPaymentAppsCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  payment_app_context->payment_app_database()->ReadAllPaymentApps(
      base::BindOnce(&DidGetAllPaymentAppsOnIO, std::move(callback)));
}

void DispatchPaymentRequestEventError(
    ServiceWorkerStatusCode service_worker_status) {
  NOTIMPLEMENTED();
}

void DispatchPaymentRequestEvent(
    payments::mojom::PaymentRequestEventDataPtr event_data,
    const PaymentAppProvider::InvokePaymentAppCallback& callback,
    const PaymentAppProvider::InvokePaymentAppStatusCallback& status_callback,
    scoped_refptr<ServiceWorkerVersion> active_version) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(active_version);

  int event_request_id = active_version->StartRequest(
      ServiceWorkerMetrics::EventType::PAYMENT_REQUEST,
      base::Bind(&DispatchPaymentRequestEventError));
  int event_finish_id = active_version->StartRequest(
      ServiceWorkerMetrics::EventType::PAYMENT_REQUEST,
      base::Bind(&ServiceWorkerUtils::NoOpStatusCallback));

  payments::mojom::PaymentHandlerInvokeCallbackPtr invoke_callback_ptr =
      InvokeCallback::Create(event_request_id, active_version, callback,
                             status_callback);
  DCHECK(invoke_callback_ptr);
  active_version->event_dispatcher()->DispatchPaymentRequestEvent(
      event_request_id, std::move(event_data), std::move(invoke_callback_ptr),
      active_version->CreateSimpleEventCallback(event_finish_id));
}

void DidFindRegistrationOnIO(
    payments::mojom::PaymentRequestEventDataPtr event_data,
    const PaymentAppProvider::InvokePaymentAppCallback& callback,
    const PaymentAppProvider::InvokePaymentAppStatusCallback& status_callback,
    ServiceWorkerStatusCode service_worker_status,
    scoped_refptr<ServiceWorkerRegistration> service_worker_registration) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (service_worker_status != SERVICE_WORKER_OK)
    return;

  ServiceWorkerVersion* active_version =
      service_worker_registration->active_version();
  DCHECK(active_version);
  active_version->RunAfterStartWorker(
      ServiceWorkerMetrics::EventType::PAYMENT_REQUEST,
      base::Bind(&DispatchPaymentRequestEvent,
                 base::Passed(std::move(event_data)), callback, status_callback,
                 make_scoped_refptr(active_version)),
      base::Bind(&DispatchPaymentRequestEventError));
}

void FindRegistrationOnIO(
    scoped_refptr<ServiceWorkerContextWrapper> service_worker_context,
    int64_t registration_id,
    payments::mojom::PaymentRequestEventDataPtr event_data,
    const PaymentAppProvider::InvokePaymentAppCallback& callback,
    const PaymentAppProvider::InvokePaymentAppStatusCallback& status_callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  service_worker_context->FindReadyRegistrationForIdOnly(
      registration_id,
      base::Bind(&DidFindRegistrationOnIO, base::Passed(std::move(event_data)),
                 callback, status_callback));
}

}  // namespace

// static
PaymentAppProvider* PaymentAppProvider::GetInstance() {
  return PaymentAppProviderImpl::GetInstance();
}

// static
PaymentAppProviderImpl* PaymentAppProviderImpl::GetInstance() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return base::Singleton<PaymentAppProviderImpl>::get();
}

void PaymentAppProviderImpl::GetAllPaymentApps(
    BrowserContext* browser_context,
    GetAllPaymentAppsCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  StoragePartitionImpl* partition = static_cast<StoragePartitionImpl*>(
      BrowserContext::GetDefaultStoragePartition(browser_context));
  scoped_refptr<PaymentAppContextImpl> payment_app_context =
      partition->GetPaymentAppContext();

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&GetAllPaymentAppsOnIO, payment_app_context,
                     std::move(callback)));
}

void PaymentAppProviderImpl::InvokePaymentApp(
    BrowserContext* browser_context,
    int64_t registration_id,
    payments::mojom::PaymentRequestEventDataPtr event_data,
    const InvokePaymentAppCallback& callback,
    const InvokePaymentAppStatusCallback& status_callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  StoragePartitionImpl* partition = static_cast<StoragePartitionImpl*>(
      BrowserContext::GetDefaultStoragePartition(browser_context));
  scoped_refptr<ServiceWorkerContextWrapper> service_worker_context =
      partition->GetServiceWorkerContext();

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&FindRegistrationOnIO, std::move(service_worker_context),
                 registration_id, base::Passed(std::move(event_data)), callback,
                 status_callback));
}

PaymentAppProviderImpl::PaymentAppProviderImpl() {}

PaymentAppProviderImpl::~PaymentAppProviderImpl() {}

}  // namespace content
