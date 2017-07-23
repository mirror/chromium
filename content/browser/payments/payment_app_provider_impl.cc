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

class ResponseCallback
    : public payments::mojom::PaymentHandlerResponseCallback {
 public:
  static payments::mojom::PaymentHandlerResponseCallbackPtr Create(
      int payment_request_id,
      scoped_refptr<ServiceWorkerVersion> service_worker_version,
      PaymentAppProvider::InvokePaymentAppCallback callback) {
    ResponseCallback* response_callback = new ResponseCallback(
        payment_request_id, std::move(service_worker_version),
        std::move(callback), PaymentAppProvider::CanMakePaymentCallback());
    payments::mojom::PaymentHandlerResponseCallbackPtr callback_proxy;
    response_callback->binding_.Bind(mojo::MakeRequest(&callback_proxy));
    return callback_proxy;
  }

  static payments::mojom::PaymentHandlerResponseCallbackPtr Create(
      int payment_request_id,
      scoped_refptr<ServiceWorkerVersion> service_worker_version,
      PaymentAppProvider::CanMakePaymentCallback callback) {
    ResponseCallback* response_callback = new ResponseCallback(
        payment_request_id, std::move(service_worker_version),
        PaymentAppProvider::InvokePaymentAppCallback(), std::move(callback));
    payments::mojom::PaymentHandlerResponseCallbackPtr callback_proxy;
    response_callback->binding_.Bind(mojo::MakeRequest(&callback_proxy));
    return callback_proxy;
  }

  ~ResponseCallback() override {}

  void OnResponseForPaymentRequest(
      payments::mojom::PaymentHandlerResponsePtr response,
      base::Time dispatch_event_time) override {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    service_worker_version_->FinishRequest(payment_request_id_, false,
                                           std::move(dispatch_event_time));
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(std::move(invoke_payment_app_callback_),
                       base::Passed(std::move(response))));
    delete this;
  }

  void OnResponseForCanMakePayment(bool can_make_payment,
                                   base::Time dispatch_event_time) override {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);
    service_worker_version_->FinishRequest(payment_request_id_, false,
                                           std::move(dispatch_event_time));
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::BindOnce(std::move(can_make_payment_callback_),
                       can_make_payment));
    delete this;
  }

 private:
  ResponseCallback(
      int payment_request_id,
      scoped_refptr<ServiceWorkerVersion> service_worker_version,
      PaymentAppProvider::InvokePaymentAppCallback invoke_payment_app_callback,
      PaymentAppProvider::CanMakePaymentCallback can_make_payment_callback)
      : payment_request_id_(payment_request_id),
        service_worker_version_(service_worker_version),
        invoke_payment_app_callback_(std::move(invoke_payment_app_callback)),
        can_make_payment_callback_(std::move(can_make_payment_callback)),
        binding_(this) {}

  int payment_request_id_;
  scoped_refptr<ServiceWorkerVersion> service_worker_version_;
  PaymentAppProvider::InvokePaymentAppCallback invoke_payment_app_callback_;
  PaymentAppProvider::CanMakePaymentCallback can_make_payment_callback_;
  mojo::Binding<payments::mojom::PaymentHandlerResponseCallback> binding_;
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
    PaymentAppProvider::InvokePaymentAppCallback callback,
    scoped_refptr<ServiceWorkerVersion> active_version) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(active_version);

  int payment_request_id = active_version->StartRequest(
      ServiceWorkerMetrics::EventType::PAYMENT_REQUEST,
      base::Bind(&DispatchPaymentRequestEventError));
  int event_finish_id = active_version->StartRequest(
      ServiceWorkerMetrics::EventType::PAYMENT_REQUEST,
      base::Bind(&ServiceWorkerUtils::NoOpStatusCallback));

  payments::mojom::PaymentHandlerResponseCallbackPtr response_callback_ptr =
      ResponseCallback::Create(payment_request_id, active_version,
                               std::move(callback));
  DCHECK(response_callback_ptr);
  active_version->event_dispatcher()->DispatchPaymentRequestEvent(
      payment_request_id, std::move(event_data),
      std::move(response_callback_ptr),
      active_version->CreateSimpleEventCallback(event_finish_id));
}

void DispatchCanMakePaymentEvent(
    payments::mojom::CanMakePaymentEventDataPtr event_data,
    PaymentAppProvider::CanMakePaymentCallback callback,
    scoped_refptr<ServiceWorkerVersion> active_version) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(active_version);

  int payment_request_id = active_version->StartRequest(
      ServiceWorkerMetrics::EventType::CAN_MAKE_PAYMENT,
      base::Bind(&DispatchPaymentRequestEventError));
  int event_finish_id = active_version->StartRequest(
      ServiceWorkerMetrics::EventType::CAN_MAKE_PAYMENT,
      base::Bind(&ServiceWorkerUtils::NoOpStatusCallback));

  payments::mojom::PaymentHandlerResponseCallbackPtr response_callback_ptr =
      ResponseCallback::Create(payment_request_id, active_version,
                               std::move(callback));
  DCHECK(response_callback_ptr);
  active_version->event_dispatcher()->DispatchCanMakePaymentEvent(
      payment_request_id, std::move(event_data),
      std::move(response_callback_ptr),
      active_version->CreateSimpleEventCallback(event_finish_id));
}

using ServiceWorkerStartCallback =
    base::OnceCallback<void(scoped_refptr<ServiceWorkerVersion>)>;

void RunAfterStartWorkerOnIO(
    scoped_refptr<ServiceWorkerVersion> service_worker_version,
    ServiceWorkerStartCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  std::move(callback).Run(std::move(service_worker_version));
}

void DidFindRegistrationOnIO(
    ServiceWorkerMetrics::EventType event_type,
    ServiceWorkerStartCallback callback,
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
      base::Bind(&RunAfterStartWorkerOnIO, make_scoped_refptr(active_version),
                 base::Passed(std::move(callback))),
      base::Bind(&DispatchPaymentRequestEventError));
}

void FindRegistrationOnIO(
    scoped_refptr<ServiceWorkerContextWrapper> service_worker_context,
    int64_t registration_id,
    ServiceWorkerMetrics::EventType event_type,
    ServiceWorkerStartCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  service_worker_context->FindReadyRegistrationForIdOnly(
      registration_id, base::Bind(&DidFindRegistrationOnIO, event_type,
                                  base::Passed(std::move(callback))));
}

void StartServiceWorkerForDispatch(ServiceWorkerMetrics::EventType event_type,
                                   BrowserContext* browser_context,
                                   int64_t registration_id,
                                   ServiceWorkerStartCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  StoragePartitionImpl* partition = static_cast<StoragePartitionImpl*>(
      BrowserContext::GetDefaultStoragePartition(browser_context));
  scoped_refptr<ServiceWorkerContextWrapper> service_worker_context =
      partition->GetServiceWorkerContext();

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&FindRegistrationOnIO, std::move(service_worker_context),
                     registration_id, event_type, std::move(callback)));
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
    InvokePaymentAppCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  StartServiceWorkerForDispatch(
      ServiceWorkerMetrics::EventType::PAYMENT_REQUEST, browser_context,
      registration_id,
      base::BindOnce(&DispatchPaymentRequestEvent,
                     base::Passed(std::move(event_data)), std::move(callback)));
}

void PaymentAppProviderImpl::CanMakePayment(
    BrowserContext* browser_context,
    int64_t registration_id,
    payments::mojom::CanMakePaymentEventDataPtr event_data,
    CanMakePaymentCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  StartServiceWorkerForDispatch(
      ServiceWorkerMetrics::EventType::CAN_MAKE_PAYMENT, browser_context,
      registration_id,
      base::BindOnce(&DispatchCanMakePaymentEvent,
                     base::Passed(std::move(event_data)), std::move(callback)));
}

PaymentAppProviderImpl::PaymentAppProviderImpl() {}

PaymentAppProviderImpl::~PaymentAppProviderImpl() {}

}  // namespace content
