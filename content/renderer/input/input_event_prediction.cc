// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/input/input_event_prediction.h"

#include "ui/events/base_event_utils.h"

using blink::WebInputEvent;
using blink::WebMouseEvent;
using blink::WebPointerProperties;
using blink::WebTouchEvent;
using blink::WebTouchPoint;

namespace content {

PointPredictor::PointPredictor() {}
PointPredictor::~PointPredictor() {}

void PointPredictor::Update(const WebPointerProperties& event,
                            double timestamp) {
  input_prediction::InputData data = {event.PositionInWidget().x,
                                      event.PositionInWidget().y, 1, timestamp};
  kalman_predictor_.Update(data);
}

void PointPredictor::ResamplePoint(WebPointerProperties& event, double time) {
  if (Stable()) {
    std::vector<input_prediction::InputData> predicted =
        kalman_predictor_.GeneratePrediction(time);
    if (!predicted.empty()) {
      event.SetPositionInWidget(predicted[0].pos_x, predicted[0].pos_y);
    }
  }
}

bool PointPredictor::Stable() {
  return kalman_predictor_.HasPrediction();
}

InputEventPredictor::InputEventPredictor() {
  pointer_type_ = WebPointerProperties::PointerType::kUnknown;
}

InputEventPredictor::~InputEventPredictor() {}

void InputEventPredictor::Update(const WebPointerProperties& event,
                                 double time) {
  pointer_predictor_map_[event.id].Update(event, time);
}

void InputEventPredictor::HandleEvent(const WebInputEvent& event) {
  if (WebInputEvent::IsTouchEventType(event.GetType())) {
    const WebTouchEvent& touch_event = static_cast<const WebTouchEvent&>(event);
    if (touch_event.touches_length > 0) {
      // If pointer type changed, assuming clear mapping and discard all stored
      // coordinates.
      if (pointer_type_ != touch_event.touches[0].pointer_type) {
        pointer_predictor_map_.clear();
        pointer_type_ = touch_event.touches[0].pointer_type;
      }

      for (unsigned i = 0; i < touch_event.touches_length; ++i) {
        if (touch_event.touches[i].state == WebTouchPoint::kStateReleased)
          pointer_predictor_map_.erase(touch_event.touches[i].id);
        else
          Update(touch_event.touches[i], touch_event.TimeStampSeconds());
      }
    }
  } else if (WebInputEvent::IsMouseEventType(event.GetType())) {
    const WebMouseEvent& mouse_event = static_cast<const WebMouseEvent&>(event);

    if (pointer_type_ != mouse_event.pointer_type) {
      pointer_predictor_map_.clear();
      pointer_type_ = mouse_event.pointer_type;
    }

    switch (mouse_event.GetType()) {
      case WebInputEvent::kMouseLeave:
        pointer_predictor_map_.erase(mouse_event.id);
        break;
      case WebInputEvent::kMouseEnter:
        pointer_predictor_map_.erase(mouse_event.id);
      case WebInputEvent::kMouseMove:
        Update(mouse_event, mouse_event.TimeStampSeconds());
        break;
      default:
        return;
    }
  }
}

// TODO: rename
void InputEventPredictor::ResampleEvent(WebInputEvent& event,
                                        base::TimeTicks frame_time) {
  double time = ui::EventTimeStampToSeconds(frame_time);
  if (event.GetType() == WebInputEvent::kTouchMove) {
    WebTouchEvent& touch_event = static_cast<WebTouchEvent&>(event);
    if (touch_event.touches_length > 0 &&
        touch_event.touches[0].pointer_type == pointer_type_) {
      for (unsigned i = 0; i < touch_event.touches_length; ++i) {
        auto predictor = pointer_predictor_map_.find(touch_event.touches[i].id);
        if (predictor != pointer_predictor_map_.end())
          predictor->second.ResamplePoint(touch_event.touches[i], time);
      }
    }
  } else if (event.GetType() == WebInputEvent::kMouseMove) {
    WebMouseEvent& mouse_event = static_cast<WebMouseEvent&>(event);
    if (mouse_event.pointer_type == pointer_type_) {
      auto predictor = pointer_predictor_map_.find(mouse_event.id);
      if (predictor != pointer_predictor_map_.end())
        predictor->second.ResamplePoint(mouse_event, time);
    }
  }
}

}  // namespace content
