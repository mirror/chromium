// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/touchpad_pinch_event_queue.h"

// TODO write tests
#if 0

#include <stddef.h>

#include <memory>
#include <utility>

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "ui/events/base_event_utils.h"

namespace content {

class TouchpadPinchEventQueueTest
    : public TouchpadPinchEventQueueClient {
 public:
  TouchpadPinchEventQueueTest() : queue_(this) {}
  }
  ~TouchpadPinchEventQueueTest() override {}

  // TouchpadPinchEventQueueClient
  void ForwardGestureEventWithLatencyInfo(
      const blink::WebGestureEvent& event,
      const ui::LatencyInfo& latency_info) override {}
  void OnMouseWheelEventAck(const MouseWheelEventWithLatencyInfo& event,
                            InputEventAckSource ack_source,
                            InputEventAckState ack_result) override {}

  void SendMouseWheelEventImmediately(
      const MouseWheelEventWithLatencyInfo& event) override {
  }

  void OnPinchEventAck(
      const ui::TouchpadPinchEvent& event) override {
  }


 protected:
  size_t queued_event_count() const { return queue_->queued_size(); }

  std::vector<std::unique_ptr<WebInputEvent>>& all_sent_events() {
    return sent_events_;
  }

  const std::unique_ptr<WebInputEvent>& sent_input_event(size_t index) {
    return sent_events_[index];
  }
  const WebGestureEvent* sent_gesture_event(size_t index) {
    return static_cast<WebGestureEvent*>(sent_events_[index].get());
  }

  const WebMouseWheelEvent& acked_event() const { return last_acked_event_; }

  size_t GetAndResetSentEventCount() {
    size_t count = sent_events_.size();
    sent_events_.clear();
    return count;
  }

  size_t GetAndResetAckedEventCount() {
    size_t count = acked_event_count_;
    acked_event_count_ = 0;
    return count;
  }

  void SendMouseWheelEventAck(InputEventAckState ack_result) {
    queue_->ProcessMouseWheelAck(InputEventAckSource::COMPOSITOR_THREAD,
                                 ack_result, ui::LatencyInfo());
  }

  std::unique_ptr<TouchpadPinchEventQueue> queue_;
  std::vector<std::unique_ptr<WebMouseWheelEvent>> sent_events_;
  size_t acked_event_count_;
  InputEventAckState last_acked_event_state_;
  WebMouseWheelEvent last_acked_event_;
};

TEST_P(TouchpadPinchEventQueueTest, Basic) {
  SendMouseWheelPossiblyIncludingPhase(
      !scroll_latching_enabled_, kWheelScrollX, kWheelScrollY,
      kWheelScrollGlobalX, kWheelScrollGlobalY, 1, 1, 0, false,
      WebMouseWheelEvent::kPhaseBegan, WebMouseWheelEvent::kPhaseNone);
  EXPECT_EQ(0U, queued_event_count());
  EXPECT_EQ(1U, GetAndResetSentEventCount());

  // The second mouse wheel should not be sent since one is already in queue.
  SendMouseWheelPossiblyIncludingPhase(
      !scroll_latching_enabled_, kWheelScrollX, kWheelScrollY,
      kWheelScrollGlobalX, kWheelScrollGlobalY, 5, 5, 0, false,
      WebMouseWheelEvent::kPhaseChanged, WebMouseWheelEvent::kPhaseNone);
  EXPECT_EQ(1U, queued_event_count());
  EXPECT_EQ(0U, GetAndResetSentEventCount());

  // Receive an ACK for the first mouse wheel event.
  SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_CONSUMED);
  EXPECT_EQ(0U, queued_event_count());
  EXPECT_EQ(1U, GetAndResetSentEventCount());
  EXPECT_EQ(1U, GetAndResetAckedEventCount());
  EXPECT_EQ(WebInputEvent::kMouseWheel, acked_event().GetType());

  // Receive an ACK for the second mouse wheel event.
  SendMouseWheelEventAck(INPUT_EVENT_ACK_STATE_CONSUMED);
  EXPECT_EQ(0U, queued_event_count());
  EXPECT_EQ(0U, GetAndResetSentEventCount());
  EXPECT_EQ(1U, GetAndResetAckedEventCount());
  EXPECT_EQ(WebInputEvent::kMouseWheel, acked_event().GetType());
}

}  // namespace content

#endif
