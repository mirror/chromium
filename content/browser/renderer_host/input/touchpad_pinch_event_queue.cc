// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/touchpad_pinch_event_queue.h"

#include "base/trace_event/trace_event.h"
#include "third_party/WebKit/public/platform/WebMouseWheelEvent.h"
#include "ui/events/base_event_utils.h"
#include "ui/events/blink/blink_event_util.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/latency/latency_info.h"

namespace content {

namespace {

blink::WebMouseWheelEvent CreateSyntheticWheelFromTouchpadPinchEvent(
    const ui::TouchpadPinchEvent& pinch_event) {
  DCHECK_EQ(ui::ET_TOUCHPAD_PINCH_UPDATE, pinch_event.type());

  // For pinch gesture events, match typical trackpad behavior on Windows by
  // sending fake wheel events with the ctrl modifier set when we see trackpad
  // pinch gestures. Ideally we'd someday get a platform 'pinch' event and
  // send that instead.
  blink::WebMouseWheelEvent wheel_event(
      blink::WebInputEvent::kMouseWheel,
      ui::EventFlagsToWebEventModifiers(pinch_event.flags() |
                                        ui::EF_CONTROL_DOWN),
      ui::EventTimeStampToSeconds(pinch_event.time_stamp()));
  wheel_event.SetPositionInWidget(pinch_event.x(), pinch_event.y());
  wheel_event.SetPositionInScreen(pinch_event.root_location_f().x(),
                                  pinch_event.root_location_f().y());
  wheel_event.delta_x = 0;

  // The function to convert scales to deltaY values is designed to be
  // compatible with websites existing use of wheel events, and with existing
  // Windows trackpad behavior. In particular, we want:
  //  - deltas should accumulate via addition: f(s1*s2)==f(s1)+f(s2)
  //  - deltas should invert via negation: f(1/s) == -f(s)
  //  - zoom in should be positive: f(s) > 0 iff s > 1
  //  - magnitude roughly matches wheels: f(2) > 25 && f(2) < 100
  //  - a formula that's relatively easy to use from JavaScript
  // Note that 'wheel' event deltaY values have their sign inverted.  So to
  // convert a wheel deltaY back to a scale use Math.exp(-deltaY/100).
  DCHECK_GT(pinch_event.scale(), 0);
  wheel_event.delta_y = 100.0f * log(pinch_event.scale());
  wheel_event.has_precise_scrolling_deltas = true;
  wheel_event.wheel_ticks_x = 0;
  wheel_event.wheel_ticks_y = pinch_event.scale() > 1 ? 1 : -1;

  return wheel_event;
}

}  // namespace

// This is a single queued guesture pinch event to which we add trace events.
class QueuedTouchpadPinchEvent : public ui::TouchpadPinchEvent {
 public:
  QueuedTouchpadPinchEvent(const ui::TouchpadPinchEvent& original_event)
      : ui::TouchpadPinchEvent(original_event) {
    TRACE_EVENT_ASYNC_BEGIN0("input", "TouchpadPinchEventQueue::QueueEvent",
                             this);
  }

  ~QueuedTouchpadPinchEvent() override {
    TRACE_EVENT_ASYNC_END0("input", "TouchpadPinchEventQueue::QueueEvent",
                           this);
  }

  // TODO only one caller for this function now, remove
  MouseWheelEventWithLatencyInfo AsWheelEvent() const {
    DCHECK_EQ(ui::ET_TOUCHPAD_PINCH_UPDATE, type());
    return MouseWheelEventWithLatencyInfo(
        CreateSyntheticWheelFromTouchpadPinchEvent(*this), *latency());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(QueuedTouchpadPinchEvent);
};

TouchpadPinchEventQueue::TouchpadPinchEventQueue(
    TouchpadPinchEventQueueClient* client)
    : client_(client) {
  DCHECK(client_);
}

TouchpadPinchEventQueue::~TouchpadPinchEventQueue() {}

void TouchpadPinchEventQueue::QueueEvent(const ui::TouchpadPinchEvent& event) {
  TRACE_EVENT0("input", "TouchpadPinchEventQueue::QueueEvent");

  pinch_queue_.push_back(std::make_unique<QueuedTouchpadPinchEvent>(event));
  TryForwardNextEventToRenderer();
}

void TouchpadPinchEventQueue::ProcessMouseWheelAck(
    InputEventAckSource ack_source,
    InputEventAckState ack_result,
    const ui::LatencyInfo& latency_info) {
  TRACE_EVENT0("input", "TouchpadPinchEventQueue::ProcessMouseWheelAck");
  if (!pinch_event_awaiting_ack_)
    return;

  pinch_event_awaiting_ack_->latency()->AddNewLatencyFrom(latency_info);

  if (ack_result == INPUT_EVENT_ACK_STATE_CONSUMED) {
    pinch_event_awaiting_ack_->SetHandled();
  }

  client_->OnPinchEventAck(*pinch_event_awaiting_ack_, ack_source, ack_result);
  pinch_event_awaiting_ack_.reset();
  TryForwardNextEventToRenderer();
}

void TouchpadPinchEventQueue::TryForwardNextEventToRenderer() {
  TRACE_EVENT0("input",
               "TouchpadPinchEventQueue::TryForwardNextEventToRenderer");

  if (pinch_queue_.empty() || pinch_event_awaiting_ack_)
    return;

  pinch_event_awaiting_ack_ = std::move(pinch_queue_.front());
  pinch_queue_.pop_front();

  if (pinch_event_awaiting_ack_->type() == ui::ET_TOUCHPAD_PINCH_BEGIN ||
      pinch_event_awaiting_ack_->type() == ui::ET_TOUCHPAD_PINCH_END) {
    // Wheel event listeners are given individual events with no phase
    // information, so we don't need to do anything at the beginning or
    // end of a pinch.
    // TODO(mcnee): Consider sending the rest of the wheel events as
    // non-blocking if the first wheel event of the pinch sequence is not
    // consumed.
    client_->OnPinchEventAck(*pinch_event_awaiting_ack_,
                             InputEventAckSource::BROWSER,
                             INPUT_EVENT_ACK_STATE_NO_CONSUMER_EXISTS);
    pinch_event_awaiting_ack_.reset();
    TryForwardNextEventToRenderer();
    return;
  }

  // TODO Do we need to worry about conflicting with the mouse wheel event
  // queue?
  client_->SendMouseWheelEventForPinchImmediately(
      pinch_event_awaiting_ack_->AsWheelEvent());
}

bool TouchpadPinchEventQueue::has_pending() const {
  return !pinch_queue_.empty() || pinch_event_awaiting_ack_;
}

}  // namespace content
