// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/service_worker_version_proxy.h"

#include "content/browser/service_worker/service_worker_version.h"

namespace content {

ServiceWorkerVersionProxy::ServiceWorkerVersionProxy(
    ServiceWorkerVersion* receiver_version)
    : receiver_version_(receiver_version) {
  DCHECK(receiver_version_);
}

ServiceWorkerVersionProxy::~ServiceWorkerVersionProxy() = default;

mojom::ServiceWorkerEventDispatcherPtrInfo
ServiceWorkerVersionProxy::CreateEventDispatcherPtrInfo() {
  mojom::ServiceWorkerEventDispatcherPtrInfo dispatcher_ptr_info;
  incoming_event_bindings_.AddBinding(this,
                                      mojo::MakeRequest(&dispatcher_ptr_info));
  return dispatcher_ptr_info;
}

void ServiceWorkerVersionProxy::DispatchInstallEvent(
    mojom::ServiceWorkerInstallEventMethodsAssociatedPtrInfo client,
    DispatchInstallEventCallback callback) {
  NOTREACHED();
}

void ServiceWorkerVersionProxy::DispatchActivateEvent(
    DispatchActivateEventCallback callback) {
  NOTREACHED();
}

void ServiceWorkerVersionProxy::DispatchBackgroundFetchAbortEvent(
    const std::string& tag,
    DispatchBackgroundFetchAbortEventCallback callback) {
  NOTREACHED();
}

void ServiceWorkerVersionProxy::DispatchBackgroundFetchClickEvent(
    const std::string& tag,
    mojom::BackgroundFetchState state,
    DispatchBackgroundFetchClickEventCallback callback) {
  NOTREACHED();
}

void ServiceWorkerVersionProxy::DispatchBackgroundFetchFailEvent(
    const std::string& tag,
    const std::vector<BackgroundFetchSettledFetch>& fetches,
    DispatchBackgroundFetchFailEventCallback callback) {
  NOTREACHED();
}

void ServiceWorkerVersionProxy::DispatchBackgroundFetchedEvent(
    const std::string& tag,
    const std::vector<BackgroundFetchSettledFetch>& fetches,
    DispatchBackgroundFetchedEventCallback callback) {
  NOTREACHED();
}

void ServiceWorkerVersionProxy::DispatchExtendableMessageEvent(
    mojom::ExtendableMessageEventPtr event,
    DispatchExtendableMessageEventCallback callback) {
  NOTREACHED();
}

void ServiceWorkerVersionProxy::DispatchFetchEvent(
    int fetch_event_id,
    const ServiceWorkerFetchRequest& request,
    mojom::FetchEventPreloadHandlePtr preload_handle,
    mojom::ServiceWorkerFetchResponseCallbackPtr response_callback,
    DispatchFetchEventCallback callback) {
  // TODO: implement.
  NOTREACHED();
}

void ServiceWorkerVersionProxy::DispatchNotificationClickEvent(
    const std::string& notification_id,
    const PlatformNotificationData& notification_data,
    int action_index,
    const base::Optional<base::string16>& reply,
    DispatchNotificationClickEventCallback callback) {
  NOTREACHED();
}

void ServiceWorkerVersionProxy::DispatchNotificationCloseEvent(
    const std::string& notification_id,
    const PlatformNotificationData& notification_data,
    DispatchNotificationCloseEventCallback callback) {
  NOTREACHED();
}

void ServiceWorkerVersionProxy::DispatchPushEvent(
    const PushEventPayload& payload,
    DispatchPushEventCallback callback) {
  NOTREACHED();
}

void ServiceWorkerVersionProxy::DispatchSyncEvent(
    const std::string& tag,
    blink::mojom::BackgroundSyncEventLastChance last_chance,
    DispatchSyncEventCallback callback) {
  NOTREACHED();
}

void ServiceWorkerVersionProxy::DispatchPaymentRequestEvent(
    int payment_request_id,
    payments::mojom::PaymentRequestEventDataPtr event_data,
    payments::mojom::PaymentHandlerResponseCallbackPtr response_callback,
    DispatchPaymentRequestEventCallback callback) {
  NOTREACHED();
}

void ServiceWorkerVersionProxy::Ping(PingCallback callback) {
  NOTREACHED();
}

}  // namespace content
