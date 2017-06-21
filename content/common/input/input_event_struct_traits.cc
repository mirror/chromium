// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/input/input_event_struct_traits.h"

#include "base/i18n/char_iterator.h"
#include "content/common/input_messages.h"
#include "third_party/WebKit/public/platform/WebKeyboardEvent.h"
#include "ui/latency/mojo/latency_info_struct_traits.h"

namespace mojo {
namespace {

void CopyString(blink::WebUChar* dst, const base::string16& text) {
  base::i18n::UTF16CharIterator iter(&text);
  size_t pos = 0;
  while (!iter.end() && pos < blink::WebKeyboardEvent::kTextLengthCap - 1) {
    dst[pos++] = iter.get();
    iter.Advance();
  }
  dst[pos] = '\0';
}

content::mojom::PointerDataPtr PointerDataFromPointerProperties(
    const blink::WebPointerProperties& pointer,
    content::mojom::MouseDataPtr mouse_data) {
  return content::mojom::PointerData::New(
      pointer.id, pointer.force, pointer.tilt_x, pointer.tilt_y,
      pointer.tangential_pressure, pointer.twist, pointer.button,
      pointer.pointer_type, pointer.movement_x, pointer.movement_y,
      pointer.PositionInWidget(), pointer.PositionInScreen(),
      std::move(mouse_data));
}

}  // namespace

blink::WebInputEvent::Type
StructTraits<content::mojom::EventDataView, InputEventUniquePtr>::type(
    const InputEventUniquePtr& event) {
  return event->web_event->GetType();
}

int32_t
StructTraits<content::mojom::EventDataView, InputEventUniquePtr>::modifiers(
    const InputEventUniquePtr& event) {
  return event->web_event->GetModifiers();
}

double StructTraits<content::mojom::EventDataView, InputEventUniquePtr>::
    timestamp_seconds(const InputEventUniquePtr& event) {
  return event->web_event->TimeStampSeconds();
}

const ui::LatencyInfo&
StructTraits<content::mojom::EventDataView, InputEventUniquePtr>::latency(
    const InputEventUniquePtr& event) {
  return event->latency_info;
}

content::mojom::KeyDataPtr
StructTraits<content::mojom::EventDataView, InputEventUniquePtr>::key_data(
    const InputEventUniquePtr& event) {
  if (!event->web_event ||
      !blink::WebInputEvent::IsKeyboardEventType(event->web_event->GetType())) {
    return nullptr;
  }

  const blink::WebKeyboardEvent* key_event =
      static_cast<const blink::WebKeyboardEvent*>(event->web_event.get());
  return content::mojom::KeyData::New(
      key_event->dom_key, key_event->dom_code, key_event->windows_key_code,
      key_event->native_key_code, key_event->is_system_key,
      key_event->is_browser_shortcut, key_event->text,
      key_event->unmodified_text);
}

content::mojom::PointerDataPtr
StructTraits<content::mojom::EventDataView, InputEventUniquePtr>::pointer_data(
    const InputEventUniquePtr& event) {
  bool is_mouse_event =
      blink::WebInputEvent::IsMouseEventType(event->web_event->GetType());
  bool is_wheel_event =
      event->web_event->GetType() == blink::WebInputEvent::Type::kMouseWheel;
  if (!event->web_event || !(is_mouse_event || is_wheel_event)) {
    return nullptr;
  }

  const blink::WebMouseEvent* mouse_event =
      static_cast<const blink::WebMouseEvent*>(event->web_event.get());

  content::mojom::WheelDataPtr wheel_data;
  if (is_wheel_event) {
    const blink::WebMouseWheelEvent* wheel_event =
        static_cast<const blink::WebMouseWheelEvent*>(mouse_event);
    wheel_data = content::mojom::WheelData::New(
        wheel_event->delta_x, wheel_event->delta_y, wheel_event->wheel_ticks_x,
        wheel_event->wheel_ticks_y, wheel_event->acceleration_ratio_x,
        wheel_event->acceleration_ratio_y, wheel_event->resending_plugin_id,
        wheel_event->phase, wheel_event->momentum_phase,
        wheel_event->scroll_by_page, wheel_event->has_precise_scrolling_deltas,
        wheel_event->dispatch_type);
  }

  return PointerDataFromPointerProperties(
      *mouse_event, content::mojom::MouseData::New(mouse_event->click_count,
                                                   std::move(wheel_data)));
}

content::mojom::TouchDataPtr
StructTraits<content::mojom::EventDataView, InputEventUniquePtr>::touch_data(
    const InputEventUniquePtr& event) {
  if (!event->web_event ||
      !blink::WebInputEvent::IsTouchEventType(event->web_event->GetType())) {
    return nullptr;
  }

  const blink::WebTouchEvent* touch_event =
      static_cast<const blink::WebTouchEvent*>(event->web_event.get());

  std::vector<content::mojom::TouchPointPtr> touches;
  for (unsigned i = 0; i < touch_event->touches_length; ++i) {
    content::mojom::PointerDataPtr pointer_data =
        PointerDataFromPointerProperties(touch_event->touches[i], nullptr);
    touches.emplace_back(content::mojom::TouchPoint::New(
        touch_event->touches[i].state, touch_event->touches[i].radius_x,
        touch_event->touches[i].radius_y,
        touch_event->touches[i].rotation_angle, std::move(pointer_data)));
  }

  return content::mojom::TouchData::New(
      touch_event->dispatch_type, touch_event->moved_beyond_slop_region,
      touch_event->touch_start_or_first_touch_move,
      touch_event->unique_touch_event_id, std::move(touches));
}

content::mojom::GestureDataPtr
StructTraits<content::mojom::EventDataView, InputEventUniquePtr>::gesture_data(
    const InputEventUniquePtr& event) {
  if (!event->web_event ||
      !blink::WebInputEvent::IsGestureEventType(event->web_event->GetType())) {
    return nullptr;
  }
  const blink::WebGestureEvent* gesture_event =
      static_cast<const blink::WebGestureEvent*>(event->web_event.get());

  content::mojom::GestureDataPtr gesture_data =
      content::mojom::GestureData::New();
  gesture_data->screen_position = gesture_event->PositionInScreen();
  gesture_data->widget_position = gesture_event->PositionInWidget();
  gesture_data->source_device = gesture_event->source_device;
  gesture_data->unique_touch_event_id = gesture_event->unique_touch_event_id;
  gesture_data->resending_plugin_id = gesture_event->resending_plugin_id;

  switch (gesture_event->GetType()) {
    default:
      break;

    case blink::WebInputEvent::Type::kGestureTapDown:
      gesture_data->contact_size =
          gfx::Size(gesture_event->data.tap_down.width,
                    gesture_event->data.tap_down.height);
      break;
    case blink::WebInputEvent::Type::kGestureShowPress:
      gesture_data->contact_size =
          gfx::Size(gesture_event->data.show_press.width,
                    gesture_event->data.show_press.height);
      break;
    case blink::WebInputEvent::Type::kGestureTap:
    case blink::WebInputEvent::Type::kGestureTapUnconfirmed:
    case blink::WebInputEvent::Type::kGestureDoubleTap:
      gesture_data->contact_size = gfx::Size(gesture_event->data.tap.width,
                                             gesture_event->data.tap.height);
      gesture_data->tap_data =
          content::mojom::TapData::New(gesture_event->data.tap.tap_count);
      break;
    case blink::WebInputEvent::Type::kGestureLongPress:
      gesture_data->contact_size =
          gfx::Size(gesture_event->data.long_press.width,
                    gesture_event->data.long_press.height);
      break;

    case blink::WebInputEvent::Type::kGestureTwoFingerTap:
      gesture_data->contact_size =
          gfx::Size(gesture_event->data.two_finger_tap.first_finger_width,
                    gesture_event->data.two_finger_tap.first_finger_height);
      break;
    case blink::WebInputEvent::Type::kGestureScrollBegin:
      gesture_data->scroll_data = content::mojom::ScrollData::New(
          gesture_event->data.scroll_begin.delta_x_hint,
          gesture_event->data.scroll_begin.delta_y_hint,
          gesture_event->data.scroll_begin.delta_hint_units,
          gesture_event->data.scroll_begin.target_viewport,
          gesture_event->data.scroll_begin.inertial_phase,
          gesture_event->data.scroll_begin.synthetic,
          gesture_event->data.scroll_begin.pointer_count, nullptr);
      break;
    case blink::WebInputEvent::Type::kGestureScrollEnd:
      gesture_data->scroll_data = content::mojom::ScrollData::New(
          0, 0, gesture_event->data.scroll_end.delta_units, false,
          gesture_event->data.scroll_end.inertial_phase,
          gesture_event->data.scroll_end.synthetic, 0, nullptr);
      break;
    case blink::WebInputEvent::Type::kGestureScrollUpdate:
      gesture_data->scroll_data = content::mojom::ScrollData::New(
          gesture_event->data.scroll_update.delta_x,
          gesture_event->data.scroll_update.delta_y,
          gesture_event->data.scroll_update.delta_units, false,
          gesture_event->data.scroll_update.inertial_phase, false, 0,
          content::mojom::ScrollUpdate::New(
              gesture_event->data.scroll_update.velocity_x,
              gesture_event->data.scroll_update.velocity_y,
              gesture_event->data.scroll_update
                  .previous_update_in_sequence_prevented,
              gesture_event->data.scroll_update.prevent_propagation));
      break;
    case blink::WebInputEvent::Type::kGestureFlingStart:
      gesture_data->fling_data = content::mojom::FlingData::New(
          gesture_event->data.fling_start.velocity_x,
          gesture_event->data.fling_start.velocity_y,
          gesture_event->data.fling_start.target_viewport, false);
      break;
    case blink::WebInputEvent::Type::kGestureFlingCancel:
      gesture_data->fling_data = content::mojom::FlingData::New(
          0, 0, gesture_event->data.fling_cancel.target_viewport,
          gesture_event->data.fling_cancel.prevent_boosting);
      break;
    case blink::WebInputEvent::Type::kGesturePinchUpdate:
      gesture_data->pinch_data = content::mojom::PinchData::New(
          gesture_event->data.pinch_update.zoom_disabled,
          gesture_event->data.pinch_update.scale);
      break;
  }

  return gesture_data;
}

bool StructTraits<content::mojom::EventDataView, InputEventUniquePtr>::Read(
    content::mojom::EventDataView event,
    InputEventUniquePtr* out) {
  DCHECK(!out->get());

  out->reset(new content::InputEvent());

  blink::WebInputEvent::Type type;
  if (!event.ReadType(&type))
    return false;

  if (blink::WebInputEvent::IsKeyboardEventType(type)) {
    content::mojom::KeyDataPtr key_data;
    if (!event.ReadKeyData<content::mojom::KeyDataPtr>(&key_data))
      return false;

    (*out)->web_event.reset(new blink::WebKeyboardEvent(
        type, event.modifiers(), event.timestamp_seconds()));

    blink::WebKeyboardEvent* key_event =
        static_cast<blink::WebKeyboardEvent*>((*out)->web_event.get());
    key_event->windows_key_code = key_data->windows_key_code;
    key_event->native_key_code = key_data->native_key_code;
    key_event->dom_code = key_data->dom_code;
    key_event->dom_key = key_data->dom_key;
    key_event->is_system_key = key_data->is_system_key;
    key_event->is_browser_shortcut = key_data->is_browser_shortcut;
    CopyString(key_event->text, key_data->text);
    CopyString(key_event->unmodified_text, key_data->unmodified_text);
  } else if (blink::WebInputEvent::IsGestureEventType(type)) {
    content::mojom::GestureDataPtr gesture_data;
    if (!event.ReadGestureData<content::mojom::GestureDataPtr>(&gesture_data))
      return false;
    (*out)->web_event.reset(new blink::WebGestureEvent(
        type, event.modifiers(), event.timestamp_seconds()));

    blink::WebGestureEvent* gesture_event =
        static_cast<blink::WebGestureEvent*>((*out)->web_event.get());
    gesture_event->x = gesture_data->widget_position.x();
    gesture_event->y = gesture_data->widget_position.y();
    gesture_event->global_x = gesture_data->screen_position.x();
    gesture_event->global_y = gesture_data->screen_position.y();
    gesture_event->source_device = gesture_data->source_device;
    gesture_event->unique_touch_event_id = gesture_data->unique_touch_event_id;
    gesture_event->resending_plugin_id = gesture_data->resending_plugin_id;

    if (gesture_data->contact_size) {
      switch (type) {
        default:
          break;

        case blink::WebInputEvent::Type::kGestureTapDown:
        case blink::WebInputEvent::Type::kGestureTapUnconfirmed:
        case blink::WebInputEvent::Type::kGestureDoubleTap:
          gesture_event->data.tap_down.width =
              gesture_data->contact_size->width();
          gesture_event->data.tap_down.height =
              gesture_data->contact_size->height();
          break;
        case blink::WebInputEvent::Type::kGestureShowPress:
          gesture_event->data.show_press.width =
              gesture_data->contact_size->width();
          gesture_event->data.show_press.height =
              gesture_data->contact_size->height();
          break;
        case blink::WebInputEvent::Type::kGestureTap:
          gesture_event->data.tap.width = gesture_data->contact_size->width();
          gesture_event->data.tap.height = gesture_data->contact_size->height();
          break;
        case blink::WebInputEvent::Type::kGestureLongPress:
          gesture_event->data.long_press.width =
              gesture_data->contact_size->width();
          gesture_event->data.long_press.height =
              gesture_data->contact_size->height();
          break;

        case blink::WebInputEvent::Type::kGestureTwoFingerTap:
          gesture_event->data.two_finger_tap.first_finger_width =
              gesture_data->contact_size->width();
          gesture_event->data.two_finger_tap.first_finger_height =
              gesture_data->contact_size->height();
          break;
        case blink::WebInputEvent::Type::kGestureScrollBegin:
          gesture_event->data.scroll_begin.delta_x_hint =
              gesture_data->scroll_data->delta_x;
          gesture_event->data.scroll_begin.delta_y_hint =
              gesture_data->scroll_data->delta_y;
          gesture_event->data.scroll_begin.delta_hint_units =
              gesture_data->scroll_data->delta_units;
          gesture_event->data.scroll_begin.target_viewport =
              gesture_data->scroll_data->target_viewport;
          gesture_event->data.scroll_begin.inertial_phase =
              gesture_data->scroll_data->inertial_phase;
          gesture_event->data.scroll_begin.synthetic =
              gesture_data->scroll_data->synthetic;
          gesture_event->data.scroll_begin.pointer_count =
              gesture_data->scroll_data->pointer_count;
          break;
        case blink::WebInputEvent::Type::kGestureScrollEnd:
          gesture_event->data.scroll_end.delta_units =
              gesture_data->scroll_data->delta_units;
          gesture_event->data.scroll_end.inertial_phase =
              gesture_data->scroll_data->inertial_phase;
          gesture_event->data.scroll_end.synthetic =
              gesture_data->scroll_data->synthetic;
          break;
        case blink::WebInputEvent::Type::kGestureScrollUpdate:
          gesture_event->data.scroll_update.delta_x =
              gesture_data->scroll_data->delta_x;
          gesture_event->data.scroll_update.delta_y =
              gesture_data->scroll_data->delta_y;
          gesture_event->data.scroll_update.delta_units =
              gesture_data->scroll_data->delta_units;
          gesture_event->data.scroll_update.inertial_phase =
              gesture_data->scroll_data->inertial_phase;
          if (gesture_data->scroll_data->update_details) {
            gesture_event->data.scroll_update.velocity_x =
                gesture_data->scroll_data->update_details->velocity_x;
            gesture_event->data.scroll_update.velocity_y =
                gesture_data->scroll_data->update_details->velocity_y;
            gesture_event->data.scroll_update
                .previous_update_in_sequence_prevented =
                gesture_data->scroll_data->update_details
                    ->previous_update_in_sequence_prevented;
            gesture_event->data.scroll_update.prevent_propagation =
                gesture_data->scroll_data->update_details->prevent_propagation;
          }
          break;
      }
    }

    if (gesture_data->scroll_data) {
      switch (type) {
        default:
          break;
        case blink::WebInputEvent::Type::kGestureScrollBegin:
          gesture_event->data.scroll_begin.delta_x_hint =
              gesture_data->scroll_data->delta_x;
          gesture_event->data.scroll_begin.delta_y_hint =
              gesture_data->scroll_data->delta_y;
          gesture_event->data.scroll_begin.delta_hint_units =
              gesture_data->scroll_data->delta_units;
          gesture_event->data.scroll_begin.target_viewport =
              gesture_data->scroll_data->target_viewport;
          gesture_event->data.scroll_begin.inertial_phase =
              gesture_data->scroll_data->inertial_phase;
          gesture_event->data.scroll_begin.synthetic =
              gesture_data->scroll_data->synthetic;
          gesture_event->data.scroll_begin.pointer_count =
              gesture_data->scroll_data->pointer_count;
          break;
        case blink::WebInputEvent::Type::kGestureScrollEnd:
          gesture_event->data.scroll_end.delta_units =
              gesture_data->scroll_data->delta_units;
          gesture_event->data.scroll_end.inertial_phase =
              gesture_data->scroll_data->inertial_phase;
          gesture_event->data.scroll_end.synthetic =
              gesture_data->scroll_data->synthetic;
          break;
        case blink::WebInputEvent::Type::kGestureScrollUpdate:
          gesture_event->data.scroll_update.delta_x =
              gesture_data->scroll_data->delta_x;
          gesture_event->data.scroll_update.delta_y =
              gesture_data->scroll_data->delta_y;
          gesture_event->data.scroll_update.delta_units =
              gesture_data->scroll_data->delta_units;
          gesture_event->data.scroll_update.inertial_phase =
              gesture_data->scroll_data->inertial_phase;
          if (gesture_data->scroll_data->update_details) {
            gesture_event->data.scroll_update.velocity_x =
                gesture_data->scroll_data->update_details->velocity_x;
            gesture_event->data.scroll_update.velocity_y =
                gesture_data->scroll_data->update_details->velocity_y;
            gesture_event->data.scroll_update
                .previous_update_in_sequence_prevented =
                gesture_data->scroll_data->update_details
                    ->previous_update_in_sequence_prevented;
            gesture_event->data.scroll_update.prevent_propagation =
                gesture_data->scroll_data->update_details->prevent_propagation;
          }
          break;
      }
    }

    if (gesture_data->fling_data) {
      switch (type) {
        default:
          break;
        case blink::WebInputEvent::Type::kGestureFlingStart:
          gesture_event->data.fling_start.velocity_x =
              gesture_data->fling_data->velocity_x;
          gesture_event->data.fling_start.velocity_y =
              gesture_data->fling_data->velocity_y;
          gesture_event->data.fling_start.target_viewport =
              gesture_data->fling_data->target_viewport;
          break;
        case blink::WebInputEvent::Type::kGestureFlingCancel:
          gesture_event->data.fling_cancel.target_viewport =
              gesture_data->fling_data->target_viewport;
          gesture_event->data.fling_cancel.prevent_boosting =
              gesture_data->fling_data->prevent_boosting;
          break;
      }
    }

    if (gesture_data->pinch_data &&
        type == blink::WebInputEvent::Type::kGesturePinchUpdate) {
      gesture_event->data.pinch_update.zoom_disabled =
          gesture_data->pinch_data->zoom_disabled;
      gesture_event->data.pinch_update.scale = gesture_data->pinch_data->scale;
    }
  } else if (blink::WebInputEvent::IsTouchEventType(type)) {
    (*out)->web_event.reset(new blink::WebTouchEvent(
        type, event.modifiers(), event.timestamp_seconds()));
  } else if (blink::WebInputEvent::IsMouseEventType(type) ||
             type == blink::WebInputEvent::Type::kMouseWheel) {
    content::mojom::PointerDataPtr pointer_data;
    if (!event.ReadPointerData<content::mojom::PointerDataPtr>(&pointer_data))
      return false;

    if (blink::WebInputEvent::IsMouseEventType(type)) {
      (*out)->web_event.reset(new blink::WebMouseEvent(
          type, event.modifiers(), event.timestamp_seconds()));
    } else {
      (*out)->web_event.reset(new blink::WebMouseWheelEvent(
          type, event.modifiers(), event.timestamp_seconds()));
    }

    blink::WebMouseEvent* mouse_event =
        static_cast<blink::WebMouseEvent*>((*out)->web_event.get());

    mouse_event->id = pointer_data->pointer_id;
    mouse_event->force = pointer_data->force;
    mouse_event->tilt_x = pointer_data->tilt_x;
    mouse_event->tilt_y = pointer_data->tilt_y;
    mouse_event->tangential_pressure = pointer_data->tangential_pressure;
    mouse_event->twist = pointer_data->twist;
    mouse_event->button = pointer_data->button;
    mouse_event->pointer_type = pointer_data->pointer_type;
    mouse_event->movement_x = pointer_data->movement_x;
    mouse_event->movement_y = pointer_data->movement_y;
    mouse_event->SetPositionInWidget(pointer_data->widget_position.x(),
                                     pointer_data->widget_position.y());
    mouse_event->SetPositionInScreen(pointer_data->screen_position.x(),
                                     pointer_data->screen_position.y());
    if (pointer_data->mouse_data) {
      mouse_event->click_count = pointer_data->mouse_data->click_count;

      if (type == blink::WebInputEvent::Type::kMouseWheel &&
          pointer_data->mouse_data->wheel_data) {
        blink::WebMouseWheelEvent* wheel_event =
            static_cast<blink::WebMouseWheelEvent*>(mouse_event);
        content::mojom::WheelDataPtr& wheel_data =
            pointer_data->mouse_data->wheel_data;
        wheel_event->delta_x = wheel_data->delta_x;
        wheel_event->delta_y = wheel_data->delta_y;
        wheel_event->wheel_ticks_x = wheel_data->wheel_ticks_x;
        wheel_event->wheel_ticks_y = wheel_data->wheel_ticks_y;
        wheel_event->acceleration_ratio_x = wheel_data->acceleration_ratio_x;
        wheel_event->acceleration_ratio_y = wheel_data->acceleration_ratio_y;
        wheel_event->resending_plugin_id = wheel_data->resending_plugin_id;
        wheel_event->phase = wheel_data->phase;
        wheel_event->momentum_phase = wheel_data->momentum_phase;
        wheel_event->scroll_by_page = wheel_data->scroll_by_page;
        wheel_event->has_precise_scrolling_deltas =
            wheel_data->has_precise_scrolling_deltas;
        wheel_event->dispatch_type = wheel_data->cancelable;
      }
    }
  } else {
    NOTREACHED();
  }

  if (!(*out)->web_event) {
    out->reset(nullptr);
    return false;
  }

  return event.ReadLatency(&((*out)->latency_info));
}

}  // namespace mojo
