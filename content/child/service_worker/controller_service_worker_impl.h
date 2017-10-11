// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_SERVICE_WORKER_CONTROLLER_SERVICE_WORKER_IMPL_H_
#define CONTENT_CHILD_SERVICE_WORKER_CONTROLLER_SERVICE_WORKER_IMPL_H_

#include <utility>
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "content/common/service_worker/controller_service_worker.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"

// S13nServiceWorker:
// An instance of this class is created on the service worker thread
// when ServiceWorkerContextClient's WorkerContextData is created.
// This implements mojom::ControllerServiceWorker and its Mojo endpoint
// is connected by each controllee and also by the ServiceWorkerProviderHost
// in the browser process.
// Subresource requests made by the controllees are sent to this class as
// Fetch events via the Mojo endpoints.
//
// TODO(kinuko): Implement self-killing timer, that does something similar to
// what ServiceWorkerVersion::StopWorkerIfIdle does in the browser process in
// non-S13n code.
class ControllerServiceWorkerImpl : public mojom::ControllerServiceWorker {
 public:
  // |context_client| should outlive |this|.
  ControllerServiceWorkerImpl(mojom::ControllerServiceWorkerRequest request,
                              ServiceWorkerContextClient* context_client);
  ~ControllerServiceWorkerImpl() override;

  // mojom::ControllerServiceWorker:
  void DispatchFetchEvent(
      const ServiceWorkerFetchRequest& request,
      mojom::ServiceWorkerFetchResponseCallbackPtr response_callback,
      DispatchFetchEventCallback callback) override;
  void Clone(mojom::ControllerServiceWorkerRequest request) override;

 private:
  class ResponseCallback;

  void DispatchFetchEventInternal(
      const ServiceWorkerFetchRequest& request,
      mojom::ServiceWorkerFetchResponseCallbackPtr response_callback);
  void DidFailToDispatchFetch(std::unique_ptr<ResponseCallback> callback,
                              ServiceWorkerStatusCode status);
  void DidDispatchFetchEvent(int event_finish_id,
                             ServiceWorkerStatusCode status,
                             base::Time dispatch_event_time);
  void CompleteDispatchFetchEvent(ServiceWorkerStatusCode status,
                                  base::Time dispatch_event_time);

  // Connected by the ServiceWorkerProviderHost in the browser process
  // and by the controllees.
  mojo::BindingSet<mojom::ControllerServiceWorker> bindings_;

  std::map<int /* request_id */, DispatchFetchEventCallback>
      fetch_event_callbacks_;
  int next_fetch_request_id_ = 0;

  base::WeakPtrFactory<ControllerServiceWorkerImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ControllerServiceWorkerImpl);
};

}  // namespace content

#endif  // CONTENT_CHILD_SERVICE_WORKER_CONTROLLER_SERVICE_WORKER_IMPL_H_
