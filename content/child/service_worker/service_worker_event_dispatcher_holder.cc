// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/service_worker/service_worker_event_dispatcher_holder.h"

namespace content {

ServiceWorkerEventDispatcherHolder::ServiceWorkerEventDispatcherHolder(
    mojom::ServiceWorkerEventDispatcherPtrInfo ptr_info) {
  event_dispatcher_.Bind(std::move(ptr_info));
}

ServiceWorkerEventDispatcherHolder::ServiceWorkerEventDispatcherHolder(
    mojom::ServiceWorkerEventDispatcher* rawptr)
    : event_dispatcher_rawptr_for_testing_(rawptr) {}

ServiceWorkerEventDispatcherHolder::~ServiceWorkerEventDispatcherHolder() =
    default;

}  // namespace content
