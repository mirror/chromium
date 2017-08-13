// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/browser_side_service_worker_event_dispatcher.h"

#include "base/callback.h"
#include "content/browser/service_worker/service_worker_metrics.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "content/common/service_worker/service_worker_utils.h"

namespace content {

// BrowserSideServiceWorkerEventDispatcher ----------------------------------

BrowserSideServiceWorkerEventDispatcher::
    BrowserSideServiceWorkerEventDispatcher(
        ServiceWorkerVersion* receiver_version)
    : receiver_version_(receiver_version), weak_factory_(this) {
  DCHECK(receiver_version_);
}

BrowserSideServiceWorkerEventDispatcher::
    ~BrowserSideServiceWorkerEventDispatcher() = default;

mojom::ServiceWorkerEventDispatcherPtrInfo
BrowserSideServiceWorkerEventDispatcher::CreateEventDispatcherPtrInfo() {
  mojom::ServiceWorkerEventDispatcherPtrInfo dispatcher_ptr_info;
  incoming_event_bindings_.AddBinding(this,
                                      mojo::MakeRequest(&dispatcher_ptr_info));
  return dispatcher_ptr_info;
}

void BrowserSideServiceWorkerEventDispatcher::DispatchInstallEvent(
    mojom::ServiceWorkerInstallEventMethodsAssociatedPtrInfo client,
    DispatchInstallEventCallback callback) {
  NOTREACHED();
}

void BrowserSideServiceWorkerEventDispatcher::DispatchActivateEvent(
    DispatchActivateEventCallback callback) {
  NOTREACHED();
}

void BrowserSideServiceWorkerEventDispatcher::DispatchBackgroundFetchAbortEvent(
    const std::string& tag,
    DispatchBackgroundFetchAbortEventCallback callback) {
  NOTREACHED();
}

void BrowserSideServiceWorkerEventDispatcher::DispatchBackgroundFetchClickEvent(
    const std::string& tag,
    mojom::BackgroundFetchState state,
    DispatchBackgroundFetchClickEventCallback callback) {
  NOTREACHED();
}

void BrowserSideServiceWorkerEventDispatcher::DispatchBackgroundFetchFailEvent(
    const std::string& tag,
    const std::vector<BackgroundFetchSettledFetch>& fetches,
    DispatchBackgroundFetchFailEventCallback callback) {
  NOTREACHED();
}

void BrowserSideServiceWorkerEventDispatcher::DispatchBackgroundFetchedEvent(
    const std::string& tag,
    const std::vector<BackgroundFetchSettledFetch>& fetches,
    DispatchBackgroundFetchedEventCallback callback) {
  NOTREACHED();
}

void BrowserSideServiceWorkerEventDispatcher::DispatchExtendableMessageEvent(
    mojom::ExtendableMessageEventPtr event,
    DispatchExtendableMessageEventCallback callback) {
  NOTIMPLEMENTED();
}

void BrowserSideServiceWorkerEventDispatcher::DispatchFetchEvent(
    int incoming_fetch_event_id,
    const ServiceWorkerFetchRequest& request,
    mojom::FetchEventPreloadHandlePtr preload_handle,
    mojom::ServiceWorkerFetchResponseCallbackPtr response_callback,
    DispatchFetchEventCallback callback) {
  // No support for ForeignFetch yet.
  DCHECK_NE(ServiceWorkerFetchType::FOREIGN_FETCH, request.fetch_type);
  // Only subresource requests should come here.
  DCHECK_NE(REQUEST_CONTEXT_FRAME_TYPE_NONE, request.frame_type);
  // TODO(kinuko): Support timeout.

  DCHECK(pending_fetch_callbacks_.find(incoming_fetch_event_id) ==
         pending_fetch_callbacks_.end());
  pending_fetch_callbacks_[incoming_fetch_event_id] = std::move(callback);
  pending_fetch_event_dispatch_[incoming_fetch_event_id] = base::BindOnce(
      &BrowserSideServiceWorkerEventDispatcher::DispatchFetchEventInternal,
      weak_factory_.GetWeakPtr(), incoming_fetch_event_id, request,
      std::move(preload_handle), std::move(response_callback));

  if (receiver_version_->running_status() == EmbeddedWorkerStatus::RUNNING) {
    RunDispatchFetchEvent(incoming_fetch_event_id);
    return;
  }
  receiver_version_->RunAfterStartWorker(
      ServiceWorkerMetrics::EventType::FETCH_SUB_RESOURCE,
      base::Bind(
          &BrowserSideServiceWorkerEventDispatcher::RunDispatchFetchEvent,
          weak_factory_.GetWeakPtr(), incoming_fetch_event_id),
      base::Bind(
          &BrowserSideServiceWorkerEventDispatcher::DidFailDispatchFetchEvent,
          weak_factory_.GetWeakPtr(), incoming_fetch_event_id));
}

void BrowserSideServiceWorkerEventDispatcher::RunDispatchFetchEvent(
    int incoming_fetch_event_id) {
  // This intermediate call is necessary to work-around non-move-only
  // callback. TODO(kinuko): Fix this.
  auto found = pending_fetch_event_dispatch_.find(incoming_fetch_event_id);
  DCHECK(found != pending_fetch_event_dispatch_.end());
  DCHECK_EQ(EmbeddedWorkerStatus::RUNNING, receiver_version_->running_status());
  base::OnceClosure closure = std::move(found->second);
  pending_fetch_event_dispatch_.erase(found);
  std::move(closure).Run();
}

void BrowserSideServiceWorkerEventDispatcher::DispatchFetchEventInternal(
    int incoming_fetch_event_id,
    const ServiceWorkerFetchRequest& request,
    mojom::FetchEventPreloadHandlePtr preload_handle,
    mojom::ServiceWorkerFetchResponseCallbackPtr response_callback) {
  int fetch_event_id = receiver_version_->StartRequest(
      ServiceWorkerMetrics::EventType::FETCH_SUB_RESOURCE,
      base::Bind(
          &BrowserSideServiceWorkerEventDispatcher::DidFailDispatchFetchEvent,
          weak_factory_.GetWeakPtr(), incoming_fetch_event_id));
  int event_finish_id = receiver_version_->StartRequest(
      ServiceWorkerMetrics::EventType::FETCH_WAITUNTIL,
      base::Bind(&ServiceWorkerUtils::NoOpStatusCallback));

  LOG(ERROR) << "** FETCH: " << request.url.spec();

  receiver_version_->event_dispatcher()->DispatchFetchEvent(
      fetch_event_id, request, std::move(preload_handle),
      std::move(response_callback),
      base::Bind(
          &BrowserSideServiceWorkerEventDispatcher::DidDispatchFetchEvent,
          weak_factory_.GetWeakPtr(), incoming_fetch_event_id,
          event_finish_id));
}

void BrowserSideServiceWorkerEventDispatcher::DispatchNotificationClickEvent(
    const std::string& notification_id,
    const PlatformNotificationData& notification_data,
    int action_index,
    const base::Optional<base::string16>& reply,
    DispatchNotificationClickEventCallback callback) {
  NOTREACHED();
}

void BrowserSideServiceWorkerEventDispatcher::DispatchNotificationCloseEvent(
    const std::string& notification_id,
    const PlatformNotificationData& notification_data,
    DispatchNotificationCloseEventCallback callback) {
  NOTREACHED();
}

void BrowserSideServiceWorkerEventDispatcher::DispatchPushEvent(
    const PushEventPayload& payload,
    DispatchPushEventCallback callback) {
  NOTREACHED();
}

void BrowserSideServiceWorkerEventDispatcher::DispatchSyncEvent(
    const std::string& tag,
    blink::mojom::BackgroundSyncEventLastChance last_chance,
    DispatchSyncEventCallback callback) {
  NOTREACHED();
}

void BrowserSideServiceWorkerEventDispatcher::DispatchAbortPaymentEvent(
    int payment_request_id,
    payments::mojom::PaymentHandlerResponseCallbackPtr response_callback,
    DispatchAbortPaymentEventCallback callback) {
  NOTREACHED();
}

void BrowserSideServiceWorkerEventDispatcher::DispatchCanMakePaymentEvent(
    int payment_request_id,
    payments::mojom::CanMakePaymentEventDataPtr event_data,
    payments::mojom::PaymentHandlerResponseCallbackPtr response_callback,
    DispatchCanMakePaymentEventCallback callback) {
  NOTREACHED();
}

void BrowserSideServiceWorkerEventDispatcher::DispatchPaymentRequestEvent(
    int payment_request_id,
    payments::mojom::PaymentRequestEventDataPtr event_data,
    payments::mojom::PaymentHandlerResponseCallbackPtr response_callback,
    DispatchPaymentRequestEventCallback callback) {
  NOTREACHED();
}

void BrowserSideServiceWorkerEventDispatcher::Ping(PingCallback callback) {
  NOTREACHED();
}

void BrowserSideServiceWorkerEventDispatcher::DidFailDispatchFetchEvent(
    int incoming_fetch_event_id,
    ServiceWorkerStatusCode status) {
  // TODO(kinuko): Should log the failures.
  DCHECK_NE(SERVICE_WORKER_OK, status);
  CompleteDispatchFetchEvent(incoming_fetch_event_id, status, base::Time());
}

void BrowserSideServiceWorkerEventDispatcher::DidDispatchFetchEvent(
    int incoming_fetch_event_id,
    int event_finish_id,
    ServiceWorkerStatusCode status,
    base::Time dispatch_event_time) {
  receiver_version_->FinishRequest(event_finish_id,
                                   status != SERVICE_WORKER_ERROR_ABORT,
                                   dispatch_event_time);
  CompleteDispatchFetchEvent(incoming_fetch_event_id, status,
                             dispatch_event_time);
}

void BrowserSideServiceWorkerEventDispatcher::CompleteDispatchFetchEvent(
    int incoming_fetch_event_id,
    ServiceWorkerStatusCode status,
    base::Time dispatch_event_time) {
  auto found = pending_fetch_callbacks_.find(incoming_fetch_event_id);
  DCHECK(found != pending_fetch_callbacks_.end());
  std::move(found->second).Run(status, dispatch_event_time);
  pending_fetch_callbacks_.erase(found);
}

}  // namespace content
