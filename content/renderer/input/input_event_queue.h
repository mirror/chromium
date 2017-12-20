// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INPUT_INPUT_EVENT_QUEUE_H_
#define CONTENT_RENDERER_INPUT_INPUT_EVENT_QUEUE_H_

#include "base/memory/ref_counted.h"
#include "base/optional.h"
#include "content/common/input/input_event_dispatch_type.h"
#include "content/public/common/input_event_ack_state.h"
#include "ui/events/blink/did_overscroll_params.h"
#include "ui/events/blink/web_input_event_traits.h"
#include "ui/latency/latency_info.h"

namespace content {

using HandledEventCallback =
    base::OnceCallback<void(InputEventAckState ack_state,
                            const ui::LatencyInfo& latency_info,
                            std::unique_ptr<ui::DidOverscrollParams>,
                            base::Optional<cc::TouchAction>)>;

class CONTENT_EXPORT InputEventQueue
    : public base::RefCountedThreadSafe<InputEventQueue> {
 public:
  // Called once the compositor has handled |event| and indicated that it is
  // a non-blocking event to be queued to main and worker threads.
  virtual void HandleEvent(ui::WebScopedInputEvent event,
                           const ui::LatencyInfo& latency,
                           InputEventDispatchType dispatch_type,
                           InputEventAckState ack_result,
                           HandledEventCallback handled_callback) = 0;
  virtual void QueueClosure(base::OnceClosure closure) = 0;

  virtual bool IsMainThreadQueue() = 0;

 protected:
  friend class base::RefCountedThreadSafe<InputEventQueue>;

  InputEventQueue() = default;
  virtual ~InputEventQueue() = default;

  DISALLOW_COPY_AND_ASSIGN(InputEventQueue);
};

}  // namespace content

#endif  // CONTENT_RENDERER_INPUT_INPUT_EVENT_QUEUE_H_
