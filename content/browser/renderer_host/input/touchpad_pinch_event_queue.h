// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_INPUT_TOUCHPAD_PINCH_EVENT_QUEUE_H_
#define CONTENT_BROWSER_RENDERER_HOST_INPUT_TOUCHPAD_PINCH_EVENT_QUEUE_H_

#include <memory>

#include "base/containers/circular_deque.h"
#include "content/browser/renderer_host/input/mouse_wheel_event_queue.h"
#include "content/common/content_export.h"
#include "content/public/common/input_event_ack_source.h"
#include "content/public/common/input_event_ack_state.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"

namespace ui {
class LatencyInfo;
class TouchpadPinchEvent;
}  // namespace ui

namespace content {

class QueuedTouchpadPinchEvent;

// Interface with which TouchpadPinchEventQueue can forward synthetic mouse
// wheel events and notify of pinch events.
class CONTENT_EXPORT TouchpadPinchEventQueueClient {
 public:
  virtual ~TouchpadPinchEventQueueClient() {}

  virtual void SendMouseWheelEventForPinchImmediately(
      const MouseWheelEventWithLatencyInfo& event) = 0;
  virtual void OnPinchEventAck(const ui::TouchpadPinchEvent& event,
                               InputEventAckSource ack_source,
                               InputEventAckState ack_result) = 0;
};

// A queue for sending synthetic mouse wheel events for touchpad pinches.
class CONTENT_EXPORT TouchpadPinchEventQueue {
 public:
  // The |client| must outlive the TouchpadPinchEventQueue.
  TouchpadPinchEventQueue(TouchpadPinchEventQueueClient* client);
  ~TouchpadPinchEventQueue();

  // Adds the given touchpad pinch |event| to the queue.
  void QueueEvent(const ui::TouchpadPinchEvent& event);

  // Notifies the queue that a synthetic mouse wheel event has been processed
  // by the renderer.
  void ProcessMouseWheelAck(InputEventAckSource ack_source,
                            InputEventAckState ack_result,
                            const ui::LatencyInfo& latency_info);

  bool has_pending() const WARN_UNUSED_RESULT;

 private:
  void TryForwardNextEventToRenderer();

  TouchpadPinchEventQueueClient* client_;

  base::circular_deque<std::unique_ptr<QueuedTouchpadPinchEvent>> pinch_queue_;
  std::unique_ptr<QueuedTouchpadPinchEvent> pinch_event_awaiting_ack_;

  DISALLOW_COPY_AND_ASSIGN(TouchpadPinchEventQueue);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_INPUT_TOUCHPAD_PINCH_EVENT_QUEUE_H_
