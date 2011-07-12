// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/c/pp_errors.h"
#include "ppapi/thunk/thunk.h"
#include "ppapi/thunk/enter.h"
#include "ppapi/thunk/ppb_input_event_api.h"
#include "ppapi/thunk/ppb_instance_api.h"
#include "ppapi/thunk/resource_creation_api.h"

namespace ppapi {
namespace thunk {

namespace {

typedef EnterFunction<PPB_Instance_FunctionAPI> EnterInstance;
typedef EnterResource<PPB_InputEvent_API> EnterInputEvent;

// InputEvent ------------------------------------------------------------------

int32_t RequestInputEvents(PP_Instance instance, uint32_t event_classes) {
  EnterInstance enter(instance, true);
  if (enter.failed())
    return PP_ERROR_BADARGUMENT;
  return enter.functions()->RequestInputEvents(instance, event_classes);
}

int32_t RequestFilteringInputEvents(PP_Instance instance,
                                    uint32_t event_classes) {
  EnterInstance enter(instance, true);
  if (enter.failed())
    return PP_ERROR_BADARGUMENT;
  return enter.functions()->RequestFilteringInputEvents(instance,
                                                        event_classes);
}

void ClearInputEventRequest(PP_Instance instance,
                            uint32_t event_classes) {
  EnterInstance enter(instance, true);
  if (enter.succeeded())
    enter.functions()->ClearInputEventRequest(instance, event_classes);
}

PP_Bool IsInputEvent(PP_Resource resource) {
  EnterInputEvent enter(resource, false);
  return enter.succeeded() ? PP_TRUE : PP_FALSE;
}

PP_InputEvent_Type GetType(PP_Resource event) {
  EnterInputEvent enter(event, true);
  if (enter.failed())
    return PP_INPUTEVENT_TYPE_UNDEFINED;
  return enter.object()->GetType();
}

PP_TimeTicks GetTimeStamp(PP_Resource event) {
  EnterInputEvent enter(event, true);
  if (enter.failed())
    return 0.0;
  return enter.object()->GetTimeStamp();
}

uint32_t GetModifiers(PP_Resource event) {
  EnterInputEvent enter(event, true);
  if (enter.failed())
    return 0;
  return enter.object()->GetModifiers();
}

const PPB_InputEvent g_ppb_input_event_thunk = {
  &RequestInputEvents,
  &RequestFilteringInputEvents,
  &ClearInputEventRequest,
  &IsInputEvent,
  &GetType,
  &GetTimeStamp,
  &GetModifiers
};

// Mouse -----------------------------------------------------------------------

PP_Bool IsMouseInputEvent(PP_Resource resource) {
  if (!IsInputEvent(resource))
    return PP_FALSE;  // Prevent warning log in GetType.
  PP_InputEvent_Type type = GetType(resource);
  return PP_FromBool(type == PP_INPUTEVENT_TYPE_MOUSEDOWN ||
                     type == PP_INPUTEVENT_TYPE_MOUSEUP ||
                     type == PP_INPUTEVENT_TYPE_MOUSEMOVE ||
                     type == PP_INPUTEVENT_TYPE_MOUSEENTER ||
                     type == PP_INPUTEVENT_TYPE_MOUSELEAVE ||
                     type == PP_INPUTEVENT_TYPE_CONTEXTMENU);
}

PP_InputEvent_MouseButton GetMouseButton(PP_Resource mouse_event) {
  EnterInputEvent enter(mouse_event, true);
  if (enter.failed())
    return PP_INPUTEVENT_MOUSEBUTTON_NONE;
  return enter.object()->GetMouseButton();
}

PP_Point GetMousePosition(PP_Resource mouse_event) {
  EnterInputEvent enter(mouse_event, true);
  if (enter.failed())
    return PP_MakePoint(0, 0);
  return enter.object()->GetMousePosition();
}

int32_t GetMouseClickCount(PP_Resource mouse_event) {
  EnterInputEvent enter(mouse_event, true);
  if (enter.failed())
    return 0;
  return enter.object()->GetMouseClickCount();
}

const PPB_MouseInputEvent g_ppb_mouse_input_event_thunk = {
  &IsMouseInputEvent,
  &GetMouseButton,
  &GetMousePosition,
  &GetMouseClickCount
};

// Wheel -----------------------------------------------------------------------

PP_Bool IsWheelInputEvent(PP_Resource resource) {
  if (!IsInputEvent(resource))
    return PP_FALSE;  // Prevent warning log in GetType.
  PP_InputEvent_Type type = GetType(resource);
  return PP_FromBool(type == PP_INPUTEVENT_TYPE_MOUSEWHEEL);
}

PP_FloatPoint GetWheelDelta(PP_Resource wheel_event) {
  EnterInputEvent enter(wheel_event, true);
  if (enter.failed())
    return PP_MakeFloatPoint(0.0f, 0.0f);
  return enter.object()->GetWheelDelta();
}

PP_FloatPoint GetWheelTicks(PP_Resource wheel_event) {
  EnterInputEvent enter(wheel_event, true);
  if (enter.failed())
    return PP_MakeFloatPoint(0.0f, 0.0f);
  return enter.object()->GetWheelTicks();
}

PP_Bool GetWheelScrollByPage(PP_Resource wheel_event) {
  EnterInputEvent enter(wheel_event, true);
  if (enter.failed())
    return PP_FALSE;
  return enter.object()->GetWheelScrollByPage();
}

const PPB_WheelInputEvent g_ppb_wheel_input_event_thunk = {
  &IsWheelInputEvent,
  &GetWheelDelta,
  &GetWheelTicks,
  &GetWheelScrollByPage
};

// Keyboard --------------------------------------------------------------------

PP_Bool IsKeyboardInputEvent(PP_Resource resource) {
  if (!IsInputEvent(resource))
    return PP_FALSE;  // Prevent warning log in GetType.
  PP_InputEvent_Type type = GetType(resource);
  return PP_FromBool(type == PP_INPUTEVENT_TYPE_KEYDOWN ||
                     type == PP_INPUTEVENT_TYPE_KEYUP ||
                     type == PP_INPUTEVENT_TYPE_CHAR);
}

uint32_t GetKeyCode(PP_Resource key_event) {
  EnterInputEvent enter(key_event, true);
  if (enter.failed())
    return 0;
  return enter.object()->GetKeyCode();
}

PP_Var GetCharacterText(PP_Resource character_event) {
  EnterInputEvent enter(character_event, true);
  if (enter.failed())
    return PP_MakeUndefined();
  return enter.object()->GetCharacterText();
}

const PPB_KeyboardInputEvent g_ppb_keyboard_input_event_thunk = {
  &IsKeyboardInputEvent,
  &GetKeyCode,
  &GetCharacterText
};

}  // namespace

const PPB_InputEvent* GetPPB_InputEvent_Thunk() {
  return &g_ppb_input_event_thunk;
}

const PPB_MouseInputEvent* GetPPB_MouseInputEvent_Thunk() {
  return &g_ppb_mouse_input_event_thunk;
}

const PPB_KeyboardInputEvent* GetPPB_KeyboardInputEvent_Thunk() {
  return &g_ppb_keyboard_input_event_thunk;
}

const PPB_WheelInputEvent* GetPPB_WheelInputEvent_Thunk() {
  return &g_ppb_wheel_input_event_thunk;
}

}  // namespace thunk
}  // namespace ppapi
