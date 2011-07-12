// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/plugins/ppapi/event_conversion.h"

#include "base/basictypes.h"
#include "base/i18n/char_iterator.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "base/utf_string_conversion_utils.h"
#include "ppapi/c/pp_input_event.h"
#include "ppapi/shared_impl/input_event_impl.h"
#include "ppapi/shared_impl/time_conversion.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"
#include "webkit/plugins/ppapi/common.h"

using ppapi::EventTimeToPPTimeTicks;
using ppapi::InputEventData;
using ppapi::PPTimeTicksToEventTime;
using WebKit::WebInputEvent;
using WebKit::WebKeyboardEvent;
using WebKit::WebMouseEvent;
using WebKit::WebMouseWheelEvent;

namespace webkit {
namespace ppapi {

namespace {

PP_InputEvent_Type ConvertEventTypes(WebInputEvent::Type wetype) {
  switch (wetype) {
    case WebInputEvent::MouseDown:
      return PP_INPUTEVENT_TYPE_MOUSEDOWN;
    case WebInputEvent::MouseUp:
      return PP_INPUTEVENT_TYPE_MOUSEUP;
    case WebInputEvent::MouseMove:
      return PP_INPUTEVENT_TYPE_MOUSEMOVE;
    case WebInputEvent::MouseEnter:
      return PP_INPUTEVENT_TYPE_MOUSEENTER;
    case WebInputEvent::MouseLeave:
      return PP_INPUTEVENT_TYPE_MOUSELEAVE;
    case WebInputEvent::ContextMenu:
      return PP_INPUTEVENT_TYPE_CONTEXTMENU;
    case WebInputEvent::MouseWheel:
      return PP_INPUTEVENT_TYPE_MOUSEWHEEL;
    case WebInputEvent::RawKeyDown:
      return PP_INPUTEVENT_TYPE_RAWKEYDOWN;
    case WebInputEvent::KeyDown:
      return PP_INPUTEVENT_TYPE_KEYDOWN;
    case WebInputEvent::KeyUp:
      return PP_INPUTEVENT_TYPE_KEYUP;
    case WebInputEvent::Char:
      return PP_INPUTEVENT_TYPE_CHAR;
    case WebInputEvent::Undefined:
    default:
      return PP_INPUTEVENT_TYPE_UNDEFINED;
  }
}

// Generates a PP_InputEvent with the fields common to all events, as well as
// the event type from the given web event. Event-specific fields will be zero
// initialized.
InputEventData GetEventWithCommonFieldsAndType(const WebInputEvent& web_event) {
  InputEventData result;
  result.event_type = ConvertEventTypes(web_event.type);
  result.event_time_stamp = EventTimeToPPTimeTicks(web_event.timeStampSeconds);
  return result;
}

void AppendKeyEvent(const WebInputEvent& event,
                    std::vector<InputEventData>* result_events) {
  const WebKeyboardEvent& key_event =
      static_cast<const WebKeyboardEvent&>(event);
  InputEventData result = GetEventWithCommonFieldsAndType(event);
  result.event_modifiers = key_event.modifiers;
  result.key_code = key_event.windowsKeyCode;
  result_events->push_back(result);
}

void AppendCharEvent(const WebInputEvent& event,
                     std::vector<InputEventData>* result_events) {
  const WebKeyboardEvent& key_event =
      static_cast<const WebKeyboardEvent&>(event);

  // This is a bit complex, the input event will normally just have one 16-bit
  // character in it, but may be zero or more than one. The text array is
  // just padded with 0 values for the unused ones, but is not necessarily
  // null-terminated.
  //
  // Here we see how many UTF-16 characters we have.
  size_t utf16_char_count = 0;
  while (utf16_char_count < WebKeyboardEvent::textLengthCap &&
         key_event.text[utf16_char_count])
    utf16_char_count++;

  // Make a separate PP_InputEvent for each Unicode character in the input.
  base::i18n::UTF16CharIterator iter(key_event.text, utf16_char_count);
  while (!iter.end()) {
    InputEventData result = GetEventWithCommonFieldsAndType(event);
    result.event_modifiers = key_event.modifiers;
    base::WriteUnicodeCharacter(iter.get(), &result.character_text);

    result_events->push_back(result);
    iter.Advance();
  }
}

void AppendMouseEvent(const WebInputEvent& event,
                      std::vector<InputEventData>* result_events) {
  COMPILE_ASSERT(static_cast<int>(WebMouseEvent::ButtonNone) ==
                 static_cast<int>(PP_INPUTEVENT_MOUSEBUTTON_NONE),
                 MouseNone);
  COMPILE_ASSERT(static_cast<int>(WebMouseEvent::ButtonLeft) ==
                 static_cast<int>(PP_INPUTEVENT_MOUSEBUTTON_LEFT),
                 MouseLeft);
  COMPILE_ASSERT(static_cast<int>(WebMouseEvent::ButtonRight) ==
                 static_cast<int>(PP_INPUTEVENT_MOUSEBUTTON_RIGHT),
                 MouseRight);
  COMPILE_ASSERT(static_cast<int>(WebMouseEvent::ButtonMiddle) ==
                 static_cast<int>(PP_INPUTEVENT_MOUSEBUTTON_MIDDLE),
                 MouseMiddle);

  const WebMouseEvent& mouse_event =
      static_cast<const WebMouseEvent&>(event);
  InputEventData result = GetEventWithCommonFieldsAndType(event);
  result.event_modifiers = mouse_event.modifiers;
  if (mouse_event.type == WebInputEvent::MouseDown ||
      mouse_event.type == WebInputEvent::MouseUp) {
    result.mouse_button =
        static_cast<PP_InputEvent_MouseButton>(mouse_event.button);
  }
  result.mouse_position.x = mouse_event.x;
  result.mouse_position.y = mouse_event.y;
  result.mouse_click_count = mouse_event.clickCount;
  result_events->push_back(result);
}

void AppendMouseWheelEvent(const WebInputEvent& event,
                           std::vector<InputEventData>* result_events) {
  const WebMouseWheelEvent& mouse_wheel_event =
      static_cast<const WebMouseWheelEvent&>(event);
  InputEventData result = GetEventWithCommonFieldsAndType(event);
  result.event_modifiers = mouse_wheel_event.modifiers;
  result.wheel_delta.x = mouse_wheel_event.deltaX;
  result.wheel_delta.y = mouse_wheel_event.deltaY;
  result.wheel_ticks.x = mouse_wheel_event.wheelTicksX;
  result.wheel_ticks.y = mouse_wheel_event.wheelTicksY;
  result.wheel_scroll_by_page = !!mouse_wheel_event.scrollByPage;
  result_events->push_back(result);
}

WebKeyboardEvent* BuildKeyEvent(const PP_InputEvent& event) {
  WebKeyboardEvent* key_event = new WebKeyboardEvent();
  switch (event.type) {
    case PP_INPUTEVENT_TYPE_RAWKEYDOWN:
      key_event->type = WebInputEvent::RawKeyDown;
      break;
    case PP_INPUTEVENT_TYPE_KEYDOWN:
      key_event->type = WebInputEvent::KeyDown;
      break;
    case PP_INPUTEVENT_TYPE_KEYUP:
      key_event->type = WebInputEvent::KeyUp;
      break;
    default:
      NOTREACHED();
  }
  key_event->timeStampSeconds = PPTimeTicksToEventTime(event.time_stamp);
  key_event->modifiers = event.u.key.modifier;
  key_event->windowsKeyCode = event.u.key.key_code;
  return key_event;
}

WebKeyboardEvent* BuildCharEvent(const PP_InputEvent& event) {
  WebKeyboardEvent* key_event = new WebKeyboardEvent();
  key_event->type = WebInputEvent::Char;
  key_event->timeStampSeconds = PPTimeTicksToEventTime(event.time_stamp);
  key_event->modifiers = event.u.character.modifier;

  // Make sure to not read beyond the buffer in case some bad code doesn't
  // NULL-terminate it (this is called from plugins).
  size_t text_length_cap = WebKeyboardEvent::textLengthCap;
  size_t text_len = 0;
  while (text_len < text_length_cap && event.u.character.text[text_len])
      text_len++;
  string16 text16 = UTF8ToUTF16(std::string(event.u.character.text, text_len));

  memset(key_event->text, 0, text_length_cap);
  memset(key_event->unmodifiedText, 0, text_length_cap);
  for (size_t i = 0;
       i < std::min(text_length_cap, text16.size());
       ++i)
    key_event->text[i] = text16[i];
  return key_event;
}

WebMouseEvent* BuildMouseEvent(const PP_InputEvent& event) {
  WebMouseEvent* mouse_event = new WebMouseEvent();
  switch (event.type) {
    case PP_INPUTEVENT_TYPE_MOUSEDOWN:
      mouse_event->type = WebInputEvent::MouseDown;
      break;
    case PP_INPUTEVENT_TYPE_MOUSEUP:
      mouse_event->type = WebInputEvent::MouseUp;
      break;
    case PP_INPUTEVENT_TYPE_MOUSEMOVE:
      mouse_event->type = WebInputEvent::MouseMove;
      break;
    case PP_INPUTEVENT_TYPE_MOUSEENTER:
      mouse_event->type = WebInputEvent::MouseEnter;
      break;
    case PP_INPUTEVENT_TYPE_MOUSELEAVE:
      mouse_event->type = WebInputEvent::MouseLeave;
      break;
    case PP_INPUTEVENT_TYPE_CONTEXTMENU:
      mouse_event->type = WebInputEvent::ContextMenu;
      break;
    default:
      NOTREACHED();
  }
  mouse_event->timeStampSeconds = PPTimeTicksToEventTime(event.time_stamp);
  mouse_event->modifiers = event.u.mouse.modifier;
  mouse_event->button =
      static_cast<WebMouseEvent::Button>(event.u.mouse.button);
  mouse_event->x = static_cast<int>(event.u.mouse.x);
  mouse_event->y = static_cast<int>(event.u.mouse.y);
  mouse_event->clickCount = event.u.mouse.click_count;
  return mouse_event;
}

WebMouseWheelEvent* BuildMouseWheelEvent(const PP_InputEvent& event) {
  WebMouseWheelEvent* mouse_wheel_event = new WebMouseWheelEvent();
  mouse_wheel_event->type = WebInputEvent::MouseWheel;
  mouse_wheel_event->timeStampSeconds =
      PPTimeTicksToEventTime(event.time_stamp);
  mouse_wheel_event->modifiers = event.u.wheel.modifier;
  mouse_wheel_event->deltaX = event.u.wheel.delta_x;
  mouse_wheel_event->deltaY = event.u.wheel.delta_y;
  mouse_wheel_event->wheelTicksX = event.u.wheel.wheel_ticks_x;
  mouse_wheel_event->wheelTicksY = event.u.wheel.wheel_ticks_y;
  mouse_wheel_event->scrollByPage = event.u.wheel.scroll_by_page;
  return mouse_wheel_event;
}

void InputEventDataToPPInputEvent(const InputEventData& data,
                                  PP_InputEvent* result) {
  memset(result, 0, sizeof(PP_InputEvent));
  result->type = data.event_type;
  result->time_stamp = data.event_time_stamp;

  switch (data.event_type) {
    case PP_INPUTEVENT_TYPE_MOUSEDOWN:
    case PP_INPUTEVENT_TYPE_MOUSEUP:
    case PP_INPUTEVENT_TYPE_MOUSEMOVE:
    case PP_INPUTEVENT_TYPE_MOUSEENTER:
    case PP_INPUTEVENT_TYPE_MOUSELEAVE:
    case PP_INPUTEVENT_TYPE_CONTEXTMENU:
      result->u.mouse.modifier = data.event_modifiers;
      result->u.mouse.button = data.mouse_button;
      result->u.mouse.x = static_cast<float>(data.mouse_position.x);
      result->u.mouse.y = static_cast<float>(data.mouse_position.y);
      result->u.mouse.click_count = data.mouse_click_count;
      break;
    case PP_INPUTEVENT_TYPE_MOUSEWHEEL:
      result->u.wheel.modifier = data.event_modifiers;
      result->u.wheel.delta_x = data.wheel_delta.x;
      result->u.wheel.delta_y = data.wheel_delta.y;
      result->u.wheel.wheel_ticks_x = data.wheel_ticks.x;
      result->u.wheel.wheel_ticks_y = data.wheel_ticks.y;
      break;
    case PP_INPUTEVENT_TYPE_RAWKEYDOWN:
    case PP_INPUTEVENT_TYPE_KEYDOWN:
    case PP_INPUTEVENT_TYPE_KEYUP:
      result->u.key.modifier = data.event_modifiers;
      result->u.key.key_code = data.key_code;
      break;
    case PP_INPUTEVENT_TYPE_CHAR:
      result->u.character.modifier = data.event_modifiers;
      base::strlcpy(result->u.character.text, data.character_text.c_str(),
                    arraysize(result->u.character.text));
      break;
    case PP_INPUTEVENT_TYPE_UNDEFINED:
      break;
  }
}

}  // namespace

void CreatePPEvent(const WebInputEvent& event,
                   std::vector<PP_InputEvent>* pp_events) {
  std::vector<InputEventData> data;
  CreateInputEventData(event, &data);

  pp_events->resize(data.size());
  for (size_t i = 0; i < data.size(); i++)
    InputEventDataToPPInputEvent(data[i], &(*pp_events)[i]);
}

void CreateInputEventData(const WebInputEvent& event,
                          std::vector<InputEventData>* result) {
  result->clear();

  switch (event.type) {
    case WebInputEvent::MouseDown:
    case WebInputEvent::MouseUp:
    case WebInputEvent::MouseMove:
    case WebInputEvent::MouseEnter:
    case WebInputEvent::MouseLeave:
    case WebInputEvent::ContextMenu:
      AppendMouseEvent(event, result);
      break;
    case WebInputEvent::MouseWheel:
      AppendMouseWheelEvent(event, result);
      break;
    case WebInputEvent::RawKeyDown:
    case WebInputEvent::KeyDown:
    case WebInputEvent::KeyUp:
      AppendKeyEvent(event, result);
      break;
    case WebInputEvent::Char:
      AppendCharEvent(event, result);
      break;
    case WebInputEvent::Undefined:
    default:
      break;
  }
}

WebInputEvent* CreateWebInputEvent(const PP_InputEvent& event) {
  scoped_ptr<WebInputEvent> web_input_event;
  switch (event.type) {
    case PP_INPUTEVENT_TYPE_UNDEFINED:
      return NULL;
    case PP_INPUTEVENT_TYPE_MOUSEDOWN:
    case PP_INPUTEVENT_TYPE_MOUSEUP:
    case PP_INPUTEVENT_TYPE_MOUSEMOVE:
    case PP_INPUTEVENT_TYPE_MOUSEENTER:
    case PP_INPUTEVENT_TYPE_MOUSELEAVE:
    case PP_INPUTEVENT_TYPE_CONTEXTMENU:
      web_input_event.reset(BuildMouseEvent(event));
      break;
    case PP_INPUTEVENT_TYPE_MOUSEWHEEL:
      web_input_event.reset(BuildMouseWheelEvent(event));
      break;
    case PP_INPUTEVENT_TYPE_RAWKEYDOWN:
    case PP_INPUTEVENT_TYPE_KEYDOWN:
    case PP_INPUTEVENT_TYPE_KEYUP:
      web_input_event.reset(BuildKeyEvent(event));
      break;
    case PP_INPUTEVENT_TYPE_CHAR:
      web_input_event.reset(BuildCharEvent(event));
      break;
  }

  return web_input_event.release();
}

PP_InputEvent_Class ClassifyInputEvent(WebInputEvent::Type type) {
  switch (type) {
    case WebInputEvent::MouseDown:
    case WebInputEvent::MouseUp:
    case WebInputEvent::MouseMove:
    case WebInputEvent::MouseEnter:
    case WebInputEvent::MouseLeave:
    case WebInputEvent::ContextMenu:
      return PP_INPUTEVENT_CLASS_MOUSE;
    case WebInputEvent::MouseWheel:
      return PP_INPUTEVENT_CLASS_WHEEL;
    case WebInputEvent::RawKeyDown:
    case WebInputEvent::KeyDown:
    case WebInputEvent::KeyUp:
    case WebInputEvent::Char:
      return PP_INPUTEVENT_CLASS_KEYBOARD;
    case WebInputEvent::Undefined:
    default:
      NOTREACHED();
      return PP_InputEvent_Class(0);
  }
}

}  // namespace ppapi
}  // namespace webkit
