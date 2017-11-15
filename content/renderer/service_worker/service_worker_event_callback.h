// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_EVENT_CALLBACK_H_
#define CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_EVENT_CALLBACK_H_

#include <unordered_map>

#include "base/bind.h"
#include "base/callback.h"
#include "base/time/time.h"
#include "content/renderer/service_worker/service_worker_event_timer.h"
#include "third_party/WebKit/public/platform/modules/serviceworker/service_worker_event_status.mojom.h"

namespace content {

template <typename CallbackType>
class ServiceWorkerEventCallback {
 public:
  ServiceWorkerEventCallback() = delete;
  ServiceWorkerEventCallback(ServiceWorkerEventCallback&& other)
      : timer_(other.timer_),
        callback_(std::move(other.callback_)),
        abort_callback_(std::move(other.abort_callback_)) {}

  template <typename... AbortArgs>
  ServiceWorkerEventCallback(ServiceWorkerEventTimer* timer,
                             CallbackType callback,
                             AbortArgs... abort_args)
      : timer_(timer),
        callback_(std::move(callback)),
        abort_callback_(
            base::BindOnce(&ServiceWorkerEventCallback<CallbackType>::Run<
                               blink::mojom::ServiceWorkerEventStatus,
                               AbortArgs...,
                               base::Time>,
                           base::Unretained(this),
                           blink::mojom::ServiceWorkerEventStatus::ABORTED,
                           abort_args...,
                           base::Time::Now())) {
    timer_->StartEvent();
  }

  ~ServiceWorkerEventCallback() {
    // Abort the event if |callback_| hasn't invoked yet.
    if (callback_)
      std::move(abort_callback_).Run();
  }

  void Abort() { std::move(abort_callback_).Run(); }

  template <typename... Args>
  void Run(Args... args) {
    // |callback_| could be null if it's aborted.
    if (!callback_) {
      DCHECK(!abort_callback_);
      return;
    }
    std::move(callback_).Run(args...);
    timer_->FinishEvent();
  }

 private:
  ServiceWorkerEventTimer* timer_;
  CallbackType callback_;
  base::OnceClosure abort_callback_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerEventCallback);
};

template <typename CallbackType, typename ResponseInterfacePtr>
class ServiceWorkerEventCallbackWithResponse {
 public:
  ServiceWorkerEventCallbackWithResponse() = delete;
  ServiceWorkerEventCallbackWithResponse(
      ServiceWorkerEventCallbackWithResponse&& other)
      : internal_(std::move(other.internal_)),
        response_interface_ptr_(std::move(other.response_interface_ptr_)) {}

  template <typename... AbortArgs>
  ServiceWorkerEventCallbackWithResponse(
      ServiceWorkerEventTimer* timer,
      CallbackType callback,
      ResponseInterfacePtr response_interface_ptr,
      AbortArgs... abort_args)
      : internal_(timer, std::move(callback), abort_args...),
        response_interface_ptr_(std::move(response_interface_ptr)) {}

  template <typename... Args>
  void Run(Args... args) {
    internal_.Run(args...);
    response_interface_ptr_.reset();
  }

  void Abort() {
    internal_.Abort();
    response_interface_ptr_.reset();
  }

  const ResponseInterfacePtr& response_interface_ptr() {
    return response_interface_ptr_;
  }

 private:
  ServiceWorkerEventCallback<CallbackType> internal_;
  ResponseInterfacePtr response_interface_ptr_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerEventCallbackWithResponse);
};

template <typename CallbackType>
class ServiceWorkerEventCallbackMap {
 public:
  ServiceWorkerEventCallbackMap(ServiceWorkerEventTimer* timer)
      : timer_(timer) {}

  ~ServiceWorkerEventCallbackMap() {
    for (auto& pair : map_)
      pair.second.Abort();
  }

  template <typename... AbortArgs>
  int Add(CallbackType callback, AbortArgs... args) {
    DCHECK_NE(next_id_, std::numeric_limits<int>::max());
    map_.insert(
        std::make_pair(next_id_, ServiceWorkerEventCallback<CallbackType>(
                                     timer_, std::move(callback), args...)));
    return next_id_++;
  }

  template <typename... Args>
  void RunWithId(int id, Args... args) {
    auto iter = map_.find(id);
    DCHECK(iter != map_.end());
    iter->second.Run(args...);
    map_.erase(iter);
  }

 private:
  ServiceWorkerEventTimer* timer_;
  int next_id_ = 0;
  std::unordered_map<int /* id */, ServiceWorkerEventCallback<CallbackType>>
      map_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerEventCallbackMap);
};

template <typename CallbackType, typename ResponseInterfacePtr>
class ServiceWorkerEventCallbackWithResponseMap {
 public:
  ServiceWorkerEventCallbackWithResponseMap(ServiceWorkerEventTimer* timer)
      : timer_(timer) {}

  ~ServiceWorkerEventCallbackWithResponseMap() {
    for (auto& pair : map_)
      pair.second.Abort();
  }

  template <typename... AbortArgs>
  int Add(CallbackType callback,
          ResponseInterfacePtr response_interface_ptr,
          AbortArgs... args) {
    DCHECK_NE(next_id_, std::numeric_limits<int>::max());
    map_.insert(std::make_pair(
        next_id_, ServiceWorkerEventCallbackWithResponse<CallbackType,
                                                         ResponseInterfacePtr>(
                      timer_, std::move(callback),
                      std::move(response_interface_ptr), args...)));
    return next_id_++;
  }

  template <typename... Args>
  void RunWithId(int id, Args... args) {
    auto iter = map_.find(id);
    DCHECK(iter != map_.end());
    iter->second.Run(args...);
    map_.erase(iter);
  }

  const ResponseInterfacePtr& GetResponseInterfacePtr(int id) {
    auto iter = map_.find(id);
    DCHECK(iter != map_.end());
    return iter->second.response_interface_ptr();
  }

 private:
  ServiceWorkerEventTimer* timer_;
  int next_id_ = 0;
  std::unordered_map<
      int /* id */,
      ServiceWorkerEventCallbackWithResponse<CallbackType,
                                             ResponseInterfacePtr>>
      map_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerEventCallbackWithResponseMap);
};

}  // namespace content

#endif  // CONTENT_RENDERER_SERVICE_WORKER_SERVICE_WORKER_EVENT_CALLBACK_H_
