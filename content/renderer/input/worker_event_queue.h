// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INPUT_WORKER_EVENT_QUEUE_H_
#define CONTENT_RENDERER_INPUT_WORKER_EVENT_QUEUE_H_

#include "base/feature_list.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "cc/input/touch_action.h"
#include "content/common/content_export.h"
#include "content/public/common/content_features.h"
#include "content/renderer/input/input_event_queue.h"
#include "content/renderer/input/main_thread_event_queue_task_list.h"
#include "content/renderer/input/scoped_web_input_event_with_latency_info.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "third_party/WebKit/public/platform/scheduler/renderer/renderer_scheduler.h"
#include "third_party/WebKit/public/web/WebWorkerEventQueue.h"
#include "ui/events/blink/web_input_event_traits.h"

namespace blink {
class WebWorkerEventQueue;
}

namespace content {

class CONTENT_EXPORT WorkerEventQueue : public InputEventQueue {
 public:
  WorkerEventQueue(blink::WebWorkerEventQueue*);

  // InputEventQueue methods:
  void HandleEvent(ui::WebScopedInputEvent event,
                   const ui::LatencyInfo& latency,
                   InputEventDispatchType dispatch_type,
                   InputEventAckState ack_result,
                   HandledEventCallback handled_callback) override;
  void QueueClosure(base::OnceClosure closure) override;
  bool IsMainThreadQueue() override;

 protected:
  ~WorkerEventQueue() override;
  blink::WebWorkerEventQueue* web_worker_queue_;

  DISALLOW_COPY_AND_ASSIGN(WorkerEventQueue);
};

}  // namespace content

#endif  // CONTENT_RENDERER_INPUT_WORKER_EVENT_QUEUE_H_
