/* Copyright (c) 2011 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef PPAPI_C_PPB_INPUT_EVENT_H_
#define PPAPI_C_PPB_INPUT_EVENT_H_

#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_point.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_time.h"
#include "ppapi/c/pp_var.h"

#define PPB_INPUT_EVENT_INTERFACE_0_1 "PPB_InputEvent;0.1"
#define PPB_INPUT_EVENT_INTERFACE PPB_INPUT_EVENT_INTERFACE_0_1

#define PPB_MOUSE_INPUT_EVENT_INTERFACE_0_1 "PPB_MouseInputEvent;0.1"
#define PPB_MOUSE_INPUT_EVENT_INTERFACE PPB_MOUSE_INPUT_EVENT_INTERFACE_0_1

#define PPB_WHEEL_INPUT_EVENT_INTERFACE_0_1 "PPB_WheelInputEvent;0.1"
#define PPB_WHEEL_INPUT_EVENT_INTERFACE PPB_WHEEL_INPUT_EVENT_INTERFACE_0_1

#define PPB_KEYBOARD_INPUT_EVENT_INTERFACE_0_1 "PPB_KeyboardInputEvent;0.1"
#define PPB_KEYBOARD_INPUT_EVENT_INTERFACE \
    PPB_KEYBOARD_INPUT_EVENT_INTERFACE_0_1

/**
 * @addtogroup Enums
 * @{
 */

/**
 * This enumeration contains the types of input events.
 */
typedef enum {
  PP_INPUTEVENT_TYPE_UNDEFINED   = -1,

  /**
   * Notification that a mouse button was pressed.
   *
   * Register for this event using the PP_INPUTEVENT_CLASS_MOUSE class.
   */
  PP_INPUTEVENT_TYPE_MOUSEDOWN   = 0,

  /**
   * Notification that a mouse button was released.
   *
   * Register for this event using the PP_INPUTEVENT_CLASS_MOUSE class.
   */
  PP_INPUTEVENT_TYPE_MOUSEUP     = 1,

  /**
   * Notification that a mouse button was moved when it is over the instance
   * or dragged out of it.
   *
   * Register for this event using the PP_INPUTEVENT_CLASS_MOUSE class.
   */
  PP_INPUTEVENT_TYPE_MOUSEMOVE   = 2,

  /**
   * Notification that the mouse entered the instance's bounds.
   *
   * Register for this event using the PP_INPUTEVENT_CLASS_MOUSE class.
   */
  PP_INPUTEVENT_TYPE_MOUSEENTER  = 3,

  /**
   * Notification that a mouse left the instance's bounds.
   *
   * Register for this event using the PP_INPUTEVENT_CLASS_MOUSE class.
   */
  PP_INPUTEVENT_TYPE_MOUSELEAVE  = 4,

  /**
   * Notification that the scroll wheel was used.
   *
   * Register for this event using the PP_INPUTEVENT_CLASS_WHEEL class.
   */
  PP_INPUTEVENT_TYPE_MOUSEWHEEL  = 5,

  /**
   * Notification that a key transitioned from "up" to "down".
   * TODO(brettw) differentiate from KEYDOWN.
   *
   * Register for this event using the PP_INPUTEVENT_CLASS_KEYBOARD class.
   */
  PP_INPUTEVENT_TYPE_RAWKEYDOWN  = 6,

  /**
   * Notification that a key was pressed. This does not necessarily correspond
   * to a character depending on the key and language. Use the
   * PP_INPUTEVENT_TYPE_CHAR for character input.
   *
   * Register for this event using the PP_INPUTEVENT_CLASS_KEYBOARD class.
   */
  PP_INPUTEVENT_TYPE_KEYDOWN     = 7,

  /**
   * Notification that a key was released.
   *
   * Register for this event using the PP_INPUTEVENT_CLASS_KEYBOARD class.
   */
  PP_INPUTEVENT_TYPE_KEYUP       = 8,

  /**
   * Notification that a character was typed. Use this for text input. Key
   * down events may generate 0, 1, or more than one character event depending
   * on the key, locale, and operating system.
   *
   * Register for this event using the PP_INPUTEVENT_CLASS_KEYBOARD class.
   */
  PP_INPUTEVENT_TYPE_CHAR        = 9,

  /**
   * TODO(brettw) when is this used?
   *
   * Register for this event using the PP_INPUTEVENT_CLASS_MOUSE class.
   */
  PP_INPUTEVENT_TYPE_CONTEXTMENU = 10
} PP_InputEvent_Type;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_InputEvent_Type, 4);

/**
 * This enumeration contains event modifier constants. Each modifier is one
 * bit. Retrieve the modifiers from an input event using the GetEventModifiers
 * function on PPB_InputEvent.
 */
typedef enum {
  PP_INPUTEVENT_MODIFIER_SHIFTKEY         = 1 << 0,
  PP_INPUTEVENT_MODIFIER_CONTROLKEY       = 1 << 1,
  PP_INPUTEVENT_MODIFIER_ALTKEY           = 1 << 2,
  PP_INPUTEVENT_MODIFIER_METAKEY          = 1 << 3,
  PP_INPUTEVENT_MODIFIER_ISKEYPAD         = 1 << 4,
  PP_INPUTEVENT_MODIFIER_ISAUTOREPEAT     = 1 << 5,
  PP_INPUTEVENT_MODIFIER_LEFTBUTTONDOWN   = 1 << 6,
  PP_INPUTEVENT_MODIFIER_MIDDLEBUTTONDOWN = 1 << 7,
  PP_INPUTEVENT_MODIFIER_RIGHTBUTTONDOWN  = 1 << 8,
  PP_INPUTEVENT_MODIFIER_CAPSLOCKKEY      = 1 << 9,
  PP_INPUTEVENT_MODIFIER_NUMLOCKKEY       = 1 << 10
} PP_InputEvent_Modifier;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_InputEvent_Modifier, 4);

/**
 * This enumeration contains constants representing each mouse button. To get
 * the mouse button for a mouse down or up event, use GetMouseButton on
 * PPB_InputEvent.
 */
typedef enum {
  PP_INPUTEVENT_MOUSEBUTTON_NONE   = -1,
  PP_INPUTEVENT_MOUSEBUTTON_LEFT   = 0,
  PP_INPUTEVENT_MOUSEBUTTON_MIDDLE = 1,
  PP_INPUTEVENT_MOUSEBUTTON_RIGHT  = 2
} PP_InputEvent_MouseButton;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_InputEvent_MouseButton, 4);

/**
 * @}
 */

typedef enum {
  /**
   * Request mouse input events.
   *
   * Normally you will request mouse events by calling RequestInputEvents().
   * The only use case for filtered events (via RequestFilteringInputEvents())
   * is for instances that have irregular outlines and you want to perform hit
   * testing, which is very uncommon. Requesting non-filtered mouse events will
   * lead to higher performance.
   */
  PP_INPUTEVENT_CLASS_MOUSE = 1 << 0,

  /**
   * Requests keyboard events. Keyboard events must be requested in filtering
   * mode via RequestFilteringInputEvents(). This is because many commands
   * should be forwarded to the page.
   *
   * A small number of tab and window management commands like Alt-F4 are never
   * sent to the page. You can not request these keyboard commands since it
   * would allow pages to trap users on a page.
   */
  PP_INPUTEVENT_CLASS_KEYBOARD = 1 << 1,

  /**
   * Identifies scroll wheel input event. Wheel events must be requested in
   * filtering mode via RequestFilteringInputEvents(). This is because many
   * wheel commands should be forwarded to the page.
   *
   * Most instances will not need this event. Consuming wheel events by
   * returning true from your filtered event handler will prevent the user from
   * scrolling the page when the mouse is over the instance which can be very
   * annoying.
   *
   * If you handle wheel events (for example, you have a document viewer which
   * the user can scroll), the recommended behavior is to return false only if
   * the wheel event actually causes your document to scroll. When the user
   * reaches the end of the document, return false to indicating that the event
   * was not handled. This will then forward the event to the containing page
   * for scrolling, producing the nested scrolling behavior users expect from
   * frames in a page.
   */
  PP_INPUTEVENT_CLASS_WHEEL = 1 << 2,

  /**
   * Identifies touch input events.
   *
   * Request touch events only if you intend to handle them. If the browser
   * knows you do not need to handle touch events, it can handle them at a
   * higher level and achieve higher performance.
   */
  PP_INPUTEVENT_CLASS_TOUCH = 1 << 3,

  /**
   * Identifies IME composition input events.
   *
   * Request this input event class if you allow on-the-spot IME input.
   */
  PP_INPUTEVENT_CLASS_IME = 1 << 4
} PP_InputEvent_Class;
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_InputEvent_Class, 4);

struct PPB_InputEvent {
  /**
   * Request that input events corresponding to the given input events are
   * delivered to the instance.
   *
   * You can not use this function to request keyboard events
   * (PP_INPUTEVENT_CLASS_KEYBOARD). You must use RequestFilteringInputEvents()
   * for this class of input.
   *
   * By default, no input events are delivered. Call this function with the
   * classes of events you are interested in to have them be delivered to
   * the instance. Calling this function will override any previous setting for
   * each specified class of input events (for example, if you previously
   * called RequestFilteringInputEvents(), this function will set those events
   * to non-filtering mode).
   *
   * Input events may have high overhead, so you should only request input
   * events that your plugin will actually handle. For example, the browser may
   * do optimizations for scroll or touch events that can be processed
   * substantially faster if it knows there are no non-default receivers for
   * that message. Requesting that such messages be delivered, even if they are
   * processed very quickly, may have a noticable effect on the performance of
   * the page.
   *
   * When requesting input events through this function, the events will be
   * delivered and <i>not</i> bubbled to the page. This means that even if you
   * aren't interested in the message, no other parts of the page will get
   * a crack at the message.
   *
   * Example:
   *   RequestInputEvents(instance, PP_INPUTEVENT_CLASS_MOUSE);
   *   RequestFilteringInputEvents(instance,
   *       PP_INPUTEVENT_CLASS_WHEEL | PP_INPUTEVENT_CLASS_KEYBOARD);
   *
   * @param instance The <code>PP_Instance</code> of the instance requesting
   * the given events.
   *
   * @param event_classes A combination of flags from PP_InputEvent_Class that
   * identifies the classes of events the instance is requesting. The flags
   * are combined by logically ORing their values.
   *
   * @return PP_OK if the operation succeeded, PP_ERROR_BADARGUMENT if instance
   * is invalid, or PP_ERROR_NOTSUPPORTED if one of the event class bits were
   * illegal. In the case of an invalid bit, all valid bits will be applied
   * and only the illegal bits will be ignored. The most common cause of a
   * PP_ERROR_NOTSUPPORTED return value is requesting keyboard events, these
   * must use RequestFilteringInputEvents().
   */
  int32_t (*RequestInputEvents)(PP_Instance instance,
                                uint32_t event_classes);

  /**
   * Request that input events corresponding to the given input events are
   * delivered to the instance for filtering.
   *
   * By default, no input events are delivered. In most cases you would
   * register to receive events by calling RequestInputEvents(). In some cases,
   * however, you may wish to filter events such that they can be bubbled up
   * to the DOM. In this case, register for those classes of events using
   * this function instead of RequestInputEvents(). Keyboard events must always
   * be registered in filtering mode.
   *
   * Filtering input events requires significantly more overhead than just
   * delivering them to the instance. As such, you should only request
   * filtering in those cases where it's absolutely necessary. The reason is
   * that it requires the browser to stop and block for the instance to handle
   * the input event, rather than sending the input event asynchronously. This
   * can have significant overhead.
   *
   * Example:
   *   RequestInputEvents(instance, PP_INPUTEVENT_CLASS_MOUSE);
   *   RequestFilteringInputEvents(instance,
   *       PP_INPUTEVENT_CLASS_WHEEL | PP_INPUTEVENT_CLASS_KEYBOARD);
   *
   * @return PP_OK if the operation succeeded, PP_ERROR_BADARGUMENT if instance
   * is invalid, or PP_ERROR_NOTSUPPORTED if one of the event class bits were
   * illegal. In the case of an invalid bit, all valid bits will be applied
   * and only the illegal bits will be ignored.
   */
  int32_t (*RequestFilteringInputEvents)(PP_Instance instance,
                                         uint32_t event_classes);

  /**
   * Request that input events corresponding to the given input classes no
   * longer be delivered to the instance.
   *
   * By default, no input events are delivered. If you have previously
   * requested input events via RequestInputEvents() or
   * RequestFilteringInputEvents(), this function will unregister handling
   * for the given instance. This will allow greater browser performance for
   * those events.
   *
   * Note that you may still get some input events after clearing the flag if
   * they were dispatched before the request was cleared. For example, if
   * there are 3 mouse move events waiting to be delivered, and you clear the
   * mouse event class during the processing of the first one, you'll still
   * receive the next two. You just won't get more events generated.
   *
   * @param instance The <code>PP_Instance</code> of the instance requesting
   * to no longer receive the given events.
   *
   * @param event_classes A combination of flags from PP_InputEvent_Class that
   * identifies the classes of events the instance is no longer interested in.
   */
  void (*ClearInputEventRequest)(PP_Instance instance,
                                 uint32_t event_classes);

  /**
   * Returns true if the given resource is a valid input event resource.
   */
  PP_Bool (*IsInputEvent)(PP_Resource resource);

  /**
   * Returns the type of input event for the given input event resource.
   * This is valid for all input events. Returns PP_INPUTEVENT_TYPE_UNDEFINED
   * if the resource is invalid.
   */
  PP_InputEvent_Type (*GetType)(PP_Resource event);

  /**
   * Returns the time that the event was generated. This will be before the
   * current time since processing and dispatching the event has some overhead.
   * Use this value to compare the times the user generated two events without
   * being sensitive to variable processing time.
   *
   * The return value is in time ticks, which is a monotonically increasing
   * clock not related to the wall clock time. It will not change if the user
   * changes their clock or daylight savings time starts, so can be reliably
   * used to compare events. This means, however, that you can't correlate
   * event times to a particular time of day on the system clock.
   */
  PP_TimeTicks (*GetTimeStamp)(PP_Resource event);

  /**
   * Returns a bitfield indicating which modifiers were down at the time of
   * the event. This is a combination of the flags in the
   * PP_InputEvent_Modifier enum.
   *
   * @return The modifiers associated with the event, or 0 if the given
   * resource is not a valid event resource.
   */
  uint32_t (*GetModifiers)(PP_Resource event);
};

struct PPB_MouseInputEvent {
  /**
   * Determines if a resource is a mouse event.
   *
   * @return PP_TRUE if the given resource is a valid mouse input event.
   */
  PP_Bool (*IsMouseInputEvent)(PP_Resource resource);

  /**
   * Returns which mouse button generated a mouse down or up event.
   *
   * @return The mouse button associated with mouse down and up events. This
   * value will be PP_EVENT_MOUSEBUTTON_NONE for mouse move, enter, and leave
   * events, and for all non-mouse events.
   */
  PP_InputEvent_MouseButton (*GetMouseButton)(PP_Resource mouse_event);

  /**
   * Returns the pixel location of a mouse input event.
   *
   * @return The point associated with the mouse event, relative to the upper-
   * left of the instance receiving the event. These values can be negative for
   * mouse drags. The return value will be (0, 0) for non-mouse events.
   */
  struct PP_Point (*GetMousePosition)(PP_Resource mouse_event);

  /**
   * TODO(brettw) figure out exactly what this means.
   */
  int32_t (*GetMouseClickCount)(PP_Resource mouse_event);
};

struct PPB_WheelInputEvent {
  /**
   * Determines if a resource is a wheel event.
   *
   * @return PP_TRUE if the given resource is a valid wheel input event.
   */
  PP_Bool (*IsWheelInputEvent)(PP_Resource resource);

  /**
   * Indicates the amount vertically and horizontally the user has requested
   * to scroll by with their mouse wheel. A scroll down or to the right (where
   * the content moves up or left) is represented as positive values, and
   * a scroll up or to the left (where the content moves down or right) is
   * represented as negative values.
   *
   * The units are either in pixels (when scroll_by_page is false) or pages
   * (when scroll_by_page is true). For example, y = -3 means scroll up 3
   * pixels when scroll_by_page is false, and scroll up 3 pages when
   * scroll_by_page is true.
   *
   * This amount is system dependent and will take into account the user's
   * preferred scroll sensitivity and potentially also nonlinear acceleration
   * based on the speed of the scrolling.
   *
   * Devices will be of varying resolution. Some mice with large detents will
   * only generate integer scroll amounts. But fractional values are also
   * possible, for example, on some trackpads and newer mice that don't have
   * "clicks".
   */
  struct PP_FloatPoint (*GetWheelDelta)(PP_Resource wheel_event);

  /**
   * The number of "clicks" of the scroll wheel that have produced the
   * event. The value may have system-specific acceleration applied to it,
   * depending on the device. The positive and negative meanings are the same
   * as for GetWheelDelta().
   *
   * If you are scrolling, you probably want to use the delta values.  These
   * tick events can be useful if you aren't doing actual scrolling and don't
   * want or pixel values. An example may be cycling between different items in
   * a game.
   *
   * You may receive fractional values for the wheel ticks if the mouse wheel
   * is high resolution or doesn't have "clicks". If your program wants
   * discrete events (as in the "picking items" example) you should accumulate
   * fractional click values from multiple messages until the total value
   * reaches positive or negative one. This should represent a similar amount
   * of scrolling as for a mouse that has a discrete mouse wheel.
   */
  struct PP_FloatPoint (*GetWheelTicks)(PP_Resource wheel_event);

  /**
   * Indicates if the scroll delta x/y indicates pages or lines to
   * scroll by.
   *
   * @return PP_TRUE if the event is a wheel event and the user is scrolling
   * by pages. PP_FALSE if not or if the resource is not a wheel event.
   */
  PP_Bool (*GetScrollByPage)(PP_Resource wheel_event);
};

struct PPB_KeyboardInputEvent {
  /**
   * Determines if a resource is a keyboard event.
   *
   * @return PP_TRUE if the given resource is a valid mouse input event.
   */
  PP_Bool (*IsKeyboardInputEvent)(PP_Resource resource);

  /**
   * Returns the DOM |keyCode| field for the keyboard event.
   * Chrome populates this with the Windows-style Virtual Key code of the key.
   */
  uint32_t (*GetKeyCode)(PP_Resource key_event);

  /**
   * Returns the typed character for the given character event.
   *
   * @return A string var representing a single typed character for character
   * input events. For non-character input events the return value will be an
   * undefined var.
   */
  struct PP_Var (*GetCharacterText)(PP_Resource character_event);
};

#endif  /* PPAPI_C_PPB_INPUT_EVENT_H_ */
