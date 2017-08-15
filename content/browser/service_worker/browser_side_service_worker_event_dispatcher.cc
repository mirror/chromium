// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/service_worker/browser_side_service_worker_event_dispatcher.h"

#include "base/callback.h"
#include "content/browser/service_worker/service_worker_version.h"
#include "content/common/service_worker/service_worker_utils.h"

namespace content {

// BrowserSideServiceWorkerEventDispatcher::ResponseCallback ----------------

// For handling Fetch event response even if the EventDispatcher has been
// destroyed.
class BrowserSideServiceWorkerEventDispatcher::ResponseCallback
    : public mojom::ServiceWorkerFetchResponseCallback {
 public:
  ResponseCallback(
      base::WeakPtr<BrowserSideServiceWorkerEventDispatcher> dispatcher,
      mojom::ServiceWorkerFetchResponseCallbackPtr forwarding_callback,
      ServiceWorkerVersion* version,
      int fetch_event_id)
      : dispatcher_(dispatcher),
        forwarding_callback_(forwarding_callback),
        version_(version),
        fetch_event_id_(fetch_event_id),
        binding_(this) {}
  ~ResponseCallback() override = default;

  void Run(int request_id,
           const ServiceWorkerResponse& response,
           base::Time dispatch_event_time) {
    // Legacy IPC callback is only for blob handling.
    DCHECK_EQ(fetch_event_id_, request_id);
    DCHECK(response.blob_uuid.size());
    HandleResponse(dispatcher_, fetch_event_id_, version_, response,
                   blink::mojom::ServiceWorkerStreamHandlePtr(),
                   SERVICE_WORKER_FETCH_EVENT_RESULT_RESPONSE,
                   dispatch_event_time);
  }

  // Implements mojom::ServiceWorkerFetchResponseCallback.
  void OnResponse(const ServiceWorkerResponse& response,
                  base::Time dispatch_event_time) override {
    HandleResponse(dispatcher_, fetch_event_id_, version_, response,
                   blink::mojom::ServiceWorkerStreamHandlePtr(),
                   SERVICE_WORKER_FETCH_EVENT_RESULT_RESPONSE,
                   dispatch_event_time);
  }
  void OnResponseStream(
      const ServiceWorkerResponse& response,
      blink::mojom::ServiceWorkerStreamHandlePtr body_as_stream,
      base::Time dispatch_event_time) override {
    HandleResponse(dispatcher_, fetch_event_id_, version_, response,
                   std::move(body_as_stream),
                   SERVICE_WORKER_FETCH_EVENT_RESULT_RESPONSE,
                   dispatch_event_time);
  }
  void OnFallback(base::Time dispatch_event_time) override {
    HandleResponse(
        dispatcher_, fetch_event_id_, version_, ServiceWorkerResponse(),
        blink::mojom::ServiceWorkerStreamHandlePtr(),
        SERVICE_WORKER_FETCH_EVENT_RESULT_FALLBACK, dispatch_event_time);
  }

  mojom::ServiceWorkerFetchResponseCallbackPtr CreateInterfacePtrAndBind() {
    mojom::ServiceWorkerFetchResponseCallbackPtr callback_ptr;
    binding_.Bind(mojo::MakeRequest(&callback_ptr));
    return callback_ptr;
  }

 private:
  static void HandleResponse(
      base::WeakPtr<BrowserSideServiceWorkerEventDispatcher> dispatcher,
      const int fetch_event_id,
      ServiceWorkerVersion* version,
      const ServiceWorkerResponse& response,
      blink::mojom::ServiceWorkerStreamHandlePtr body_as_stream,
      ServiceWorkerFetchEventResult fetch_result,
      base::Time dispatch_event_time) {
    // FinishRequest will delete |this|.
    if (!version_->FinishResult(
            fetch_event_id,
            fetch_result == SERVICE_WORKER_FETCH_EVENT_RESULT_RESPONSE,
            dispatch_event_time)) {
      NOTREACHED() << "Should only receive one reply per event".
    }
    if (!dispatcher)
      return;
    dispatcher->DidFinishFetchEvent(fetch_event_id, fetch_result, response,
                                    std::move(body_as_stream));
  }

  base::WeakPtr<BrowserSideServiceWorkerEventDispatcher> dispatcher_;
  mojom::ServiceWorkerFetchResponseCallbackPtr forwarding_callback_;
  ServiceWorkerVersion* version_;  // Owns |this|.
  const int fetch_event_id_;
  mojo::Binding<mojom::ServiceWorkerFetchResponseCallback> binding_;
  DISALLOW_COPY_AND_ASSIGN(ResponseCallback);
};

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
  DVLOG(1) << "FetchEvent [" << incoming_fetch_event_id << "] "
           << request.url.spec();
  DCHECK(fetch_event_callbacks_.find(incoming_fetch_event_id) ==
         fetch_event_callbacks_.end());
  fetch_event_callbacks_[incoming_fetch_event_id] = std::move(callback);

  if (receiver_version_->running_status() == EmbeddedWorkerStatus::RUNNING) {
    DispatchFetchEventInternal(incoming_fetch_event_id, request,
                               std::move(preload_handle),
                               std::move(response_callback));
    return;
  }

  // No support for ForeignFetch yet.
  DCHECK_NE(ServiceWorkerFetchType::FOREIGN_FETCH, request.fetch_type);
  // Only subresource requests should come here (as the other end of pipe for
  // this event dispatcher is passed to the renderer's SubresourceLoader).
  DCHECK_EQ(REQUEST_CONTEXT_FRAME_TYPE_NONE, request.frame_type);

  receiver_version_->RunAfterStartWorker(
      ServiceWorkerMetrics::EventType::FETCH_SUB_RESOURCE,
      base::Bind(
          &BrowserSideServiceWorkerEventDispatcher::DispatchFetchEventInternal,
          weak_factory_.GetWeakPtr(), incoming_fetch_event_id, request,
          base::Passed(std::move(preload_handle)),
          base::Passed(std::move(response_callback))),
      base::Bind(&BrowserSideServiceWorkerEventDispatcher::DidFailStartWorker,
                 weak_factory_.GetWeakPtr(), incoming_fetch_event_id));
}

void BrowserSideServiceWorkerEventDispatcher::DispatchFetchEventInternal(
    int incoming_fetch_event_id,
    const ServiceWorkerFetchRequest& request,
    mojom::FetchEventPreloadHandlePtr preload_handle,
    mojom::ServiceWorkerFetchResponseCallbackPtr response_callback) {
  // TODO(kinuko): No timeout support for now; support timeout.
  int fetch_event_id = receiver_version_->StartRequest(
      ServiceWorkerMetrics::EventType::FETCH_SUB_RESOURCE,
      base::Bind(&BrowserSideServiceWorkerEventDispatcher::OnRequestError,
                 weak_factory_.GetWeakPtr(), incoming_fetch_event_id,
                 ServiceWorkerMetrics::EventType::FETCH_SUB_RESOURCE));
  int event_finish_id = receiver_version_->StartRequest(
      ServiceWorkerMetrics::EventType::FETCH_WAITUNTIL,
      base::Bind(&BrowserSideServiceWorkerEventDispatcher::OnRequestError,
                 weak_factory_.GetWeakPtr(), incoming_fetch_event_id,
                 ServiceWorkerMetrics::EventType::FETCH_WAITUNTIL));

  receiver_version_->event_dispatcher()->DispatchFetchEvent(
      fetch_event_id, request, std::move(preload_handle),
      std::move(response_callback),
      base::Bind(
          &BrowserSideServiceWorkerEventDispatcher::DidDispatchFetchEvent,
          weak_factory_.GetWeakPtr(), incoming_fetch_event_id, fetch_event_id,
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

void BrowserSideServiceWorkerEventDispatcher::DidFailStartWorker(
    int incoming_fetch_event_id,
    ServiceWorkerStatusCode status) {
  // TODO(kinuko): Should log the failures.
  DCHECK_NE(SERVICE_WORKER_OK, status);
  CompleteDispatchFetchEvent(incoming_fetch_event_id, status, base::Time());
}

void BrowserSideServiceWorkerEventDispatcher::OnRequestError(
    int incoming_fetch_event_id,
    ServiceWorkerMetrics::EventType event_type,
    ServiceWorkerStatusCode status) {
  // TODO(kinuko): Should log the failures.
  DCHECK_NE(SERVICE_WORKER_OK, status);
}

void BrowserSideServiceWorkerEventDispatcher::DidDispatchFetchEvent(
    int incoming_fetch_event_id,
    int fetch_event_id,
    int event_finish_id,
    ServiceWorkerStatusCode status,
    base::Time dispatch_event_time) {
  receiver_version_->FinishRequest(event_finish_id,
                                   status != SERVICE_WORKER_ERROR_ABORT,
                                   dispatch_event_time);
  // TODO(kinuko): This is not right, we actually want to call this
  // when response_callback is called but we're transferring the
  // other end to the page so we can't.
  receiver_version_->FinishRequest(fetch_event_id,
                                   status != SERVICE_WORKER_ERROR_ABORT,
                                   dispatch_event_time);
  CompleteDispatchFetchEvent(incoming_fetch_event_id, status,
                             dispatch_event_time);
}

void BrowserSideServiceWorkerEventDispatcher::CompleteDispatchFetchEvent(
    int incoming_fetch_event_id,
    ServiceWorkerStatusCode status,
    base::Time dispatch_event_time) {
  DVLOG(1) << "CompleteFetch [" << incoming_fetch_event_id
           << "] status:" << status;
  auto found = fetch_event_callbacks_.find(incoming_fetch_event_id);
  DCHECK(found != fetch_event_callbacks_.end());
  std::move(found->second).Run(status, dispatch_event_time);
  fetch_event_callbacks_.erase(found);
}

}  // namespace content
