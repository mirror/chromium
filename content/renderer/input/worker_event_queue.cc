// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/input/worker_event_queue.h"

namespace content {

WorkerEventQueue::WorkerEventQueue(blink::WebWorkerEventQueue* web_worker_queue)
    : web_worker_queue_(web_worker_queue) {}

WorkerEventQueue::~WorkerEventQueue() {}

void WorkerEventQueue::HandleEvent(
    ui::WebScopedInputEvent event,
    const ui::LatencyInfo& latency,
    InputEventDispatchType original_dispatch_type,
    InputEventAckState ack_result,
    HandledEventCallback callback) {
  if (web_worker_queue_)
    web_worker_queue_->HandleInputEvent();
}

void WorkerEventQueue::QueueClosure(base::OnceClosure closure) {}

bool WorkerEventQueue::IsMainThreadQueue() {
  return false;
}

}  // namespace content
