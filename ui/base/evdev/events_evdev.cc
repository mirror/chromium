// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/events.h"

#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/XF86keysym.h>

#include "base/logging.h"
#include "base/message_pump_evdev.h"
#include "base/stringprintf.h"
#include "ui/gfx/point.h"

namespace ui {

namespace {

const int kMouseButton = (EF_LEFT_MOUSE_BUTTON |
                          EF_MIDDLE_MOUSE_BUTTON |
                          EF_RIGHT_MOUSE_BUTTON);

}  // namespace

KeyboardCode KeyboardCodeFromXKeysym(unsigned int keysym) {
  // TODO(sad): Have |keysym| go through the X map list?

  switch (keysym) {
    case XK_BackSpace:
      return VKEY_BACK;
    case XK_Delete:
    case XK_KP_Delete:
      return VKEY_DELETE;
    case XK_Tab:
    case XK_KP_Tab:
    case XK_ISO_Left_Tab:
      return VKEY_TAB;
    case XK_Linefeed:
    case XK_Return:
    case XK_KP_Enter:
    case XK_ISO_Enter:
      return VKEY_RETURN;
    case XK_Clear:
      return VKEY_CLEAR;
    case XK_KP_Space:
    case XK_space:
      return VKEY_SPACE;
    case XK_Home:
    case XK_KP_Home:
      return VKEY_HOME;
    case XK_End:
    case XK_KP_End:
      return VKEY_END;
    case XK_Page_Up:
    case XK_KP_Page_Up:
      return VKEY_PRIOR;
    case XK_Page_Down:
    case XK_KP_Page_Down:
      return VKEY_NEXT;
    case XK_Left:
    case XK_KP_Left:
      return VKEY_LEFT;
    case XK_Right:
    case XK_KP_Right:
      return VKEY_RIGHT;
    case XK_Down:
    case XK_KP_Down:
      return VKEY_DOWN;
    case XK_Up:
    case XK_KP_Up:
      return VKEY_UP;
    case XK_Escape:
      return VKEY_ESCAPE;
    case XK_Kana_Lock:
    case XK_Kana_Shift:
      return VKEY_KANA;
    case XK_Hangul:
      return VKEY_HANGUL;
    case XK_Hangul_Hanja:
      return VKEY_HANJA;
    case XK_Kanji:
      return VKEY_KANJI;
    case XK_Henkan:
      return VKEY_CONVERT;
    case XK_Muhenkan:
      return VKEY_NONCONVERT;
    case XK_A:
    case XK_a:
      return VKEY_A;
    case XK_B:
    case XK_b:
      return VKEY_B;
    case XK_C:
    case XK_c:
      return VKEY_C;
    case XK_D:
    case XK_d:
      return VKEY_D;
    case XK_E:
    case XK_e:
      return VKEY_E;
    case XK_F:
    case XK_f:
      return VKEY_F;
    case XK_G:
    case XK_g:
      return VKEY_G;
    case XK_H:
    case XK_h:
      return VKEY_H;
    case XK_I:
    case XK_i:
      return VKEY_I;
    case XK_J:
    case XK_j:
      return VKEY_J;
    case XK_K:
    case XK_k:
      return VKEY_K;
    case XK_L:
    case XK_l:
      return VKEY_L;
    case XK_M:
    case XK_m:
      return VKEY_M;
    case XK_N:
    case XK_n:
      return VKEY_N;
    case XK_O:
    case XK_o:
      return VKEY_O;
    case XK_P:
    case XK_p:
      return VKEY_P;
    case XK_Q:
    case XK_q:
      return VKEY_Q;
    case XK_R:
    case XK_r:
      return VKEY_R;
    case XK_S:
    case XK_s:
      return VKEY_S;
    case XK_T:
    case XK_t:
      return VKEY_T;
    case XK_U:
    case XK_u:
      return VKEY_U;
    case XK_V:
    case XK_v:
      return VKEY_V;
    case XK_W:
    case XK_w:
      return VKEY_W;
    case XK_X:
    case XK_x:
      return VKEY_X;
    case XK_Y:
    case XK_y:
      return VKEY_Y;
    case XK_Z:
    case XK_z:
      return VKEY_Z;

    case XK_0:
    case XK_1:
    case XK_2:
    case XK_3:
    case XK_4:
    case XK_5:
    case XK_6:
    case XK_7:
    case XK_8:
    case XK_9:
      return static_cast<KeyboardCode>(VKEY_0 + (keysym - XK_0));

    case XK_parenright:
      return VKEY_0;
    case XK_exclam:
      return VKEY_1;
    case XK_at:
      return VKEY_2;
    case XK_numbersign:
      return VKEY_3;
    case XK_dollar:
      return VKEY_4;
    case XK_percent:
      return VKEY_5;
    case XK_asciicircum:
      return VKEY_6;
    case XK_ampersand:
      return VKEY_7;
    case XK_asterisk:
      return VKEY_8;
    case XK_parenleft:
      return VKEY_9;

    case XK_KP_0:
    case XK_KP_1:
    case XK_KP_2:
    case XK_KP_3:
    case XK_KP_4:
    case XK_KP_5:
    case XK_KP_6:
    case XK_KP_7:
    case XK_KP_8:
    case XK_KP_9:
      return static_cast<KeyboardCode>(VKEY_NUMPAD0 + (keysym - XK_KP_0));

    case XK_multiply:
    case XK_KP_Multiply:
      return VKEY_MULTIPLY;
    case XK_KP_Add:
      return VKEY_ADD;
    case XK_KP_Separator:
      return VKEY_SEPARATOR;
    case XK_KP_Subtract:
      return VKEY_SUBTRACT;
    case XK_KP_Decimal:
      return VKEY_DECIMAL;
    case XK_KP_Divide:
      return VKEY_DIVIDE;
    case XK_equal:
    case XK_plus:
      return VKEY_OEM_PLUS;
    case XK_comma:
    case XK_less:
      return VKEY_OEM_COMMA;
    case XK_minus:
    case XK_underscore:
      return VKEY_OEM_MINUS;
    case XK_greater:
    case XK_period:
      return VKEY_OEM_PERIOD;
    case XK_colon:
    case XK_semicolon:
      return VKEY_OEM_1;
    case XK_question:
    case XK_slash:
      return VKEY_OEM_2;
    case XK_asciitilde:
    case XK_quoteleft:
      return VKEY_OEM_3;
    case XK_bracketleft:
    case XK_braceleft:
      return VKEY_OEM_4;
    case XK_backslash:
    case XK_bar:
      return VKEY_OEM_5;
    case XK_bracketright:
    case XK_braceright:
      return VKEY_OEM_6;
    case XK_quoteright:
    case XK_quotedbl:
      return VKEY_OEM_7;
    case XK_Shift_L:
    case XK_Shift_R:
      return VKEY_SHIFT;
    case XK_Control_L:
    case XK_Control_R:
      return VKEY_CONTROL;
    case XK_Alt_L:
    case XK_Alt_R:
      return VKEY_MENU;
    case XK_Pause:
      return VKEY_PAUSE;
    case XK_Caps_Lock:
      return VKEY_CAPITAL;
    case XK_Num_Lock:
      return VKEY_NUMLOCK;
    case XK_Scroll_Lock:
      return VKEY_SCROLL;
    case XK_Select:
      return VKEY_SELECT;
    case XK_Print:
      return VKEY_PRINT;
    case XK_Execute:
      return VKEY_EXECUTE;
    case XK_Insert:
    case XK_KP_Insert:
      return VKEY_INSERT;
    case XK_Help:
      return VKEY_HELP;
    case XK_Super_L:
      return VKEY_LWIN;
    case XK_Super_R:
      return VKEY_RWIN;
    case XK_Menu:
      return VKEY_APPS;
    case XK_F1:
    case XK_F2:
    case XK_F3:
    case XK_F4:
    case XK_F5:
    case XK_F6:
    case XK_F7:
    case XK_F8:
    case XK_F9:
    case XK_F10:
    case XK_F11:
    case XK_F12:
    case XK_F13:
    case XK_F14:
    case XK_F15:
    case XK_F16:
    case XK_F17:
    case XK_F18:
    case XK_F19:
    case XK_F20:
    case XK_F21:
    case XK_F22:
    case XK_F23:
    case XK_F24:
      return static_cast<KeyboardCode>(VKEY_F1 + (keysym - XK_F1));

#if defined(TOOLKIT_USES_GTK)
    case XF86XK_HomePage:
    case XF86XK_Search:
    case XF86XK_Back:
    case XF86XK_Forward:
    case XF86XK_Stop:
    case XF86XK_Refresh:
    case XF86XK_Favorites:
    case XF86XK_History:
    case XF86XK_OpenURL:
    case XF86XK_AddFavorite:
    case XF86XK_Go:
    case XF86XK_Reload:
    case XF86XK_ZoomIn:
    case XF86XK_ZoomOut:
      // ui::AcceleratorGtk tries to convert the XF86XK_ keysyms on Chrome
      // startup. It's safe to return VKEY_UNKNOWN here since ui::AcceleratorGtk
      // also checks a Gdk keysym. http://crbug.com/109843
      return VKEY_UNKNOWN;
#endif

    // For supporting multimedia buttons on a USB keyboard.
    case XF86XK_AudioMute:
      return VKEY_VOLUME_MUTE;
    case XF86XK_AudioLowerVolume:
      return VKEY_VOLUME_DOWN;
    case XF86XK_AudioRaiseVolume:
      return VKEY_VOLUME_UP;

    // TODO(sad): some keycodes are still missing.
  }
  DLOG(WARNING) << "Unknown keysym: " << StringPrintf("0x%x", keysym);
  return VKEY_UNKNOWN;
}

EventType EventTypeFromNative(const base::NativeEvent& event) {
  switch (event->type) {
  case base::EVDEV_EVENT_RELATIVE_MOTION:
    return event->modifiers & kMouseButton ? ET_MOUSE_DRAGGED : ET_MOUSE_MOVED;
  case base::EVDEV_EVENT_BUTTON:
    return event->value ? ET_MOUSE_PRESSED : ET_MOUSE_RELEASED;
  case base::EVDEV_EVENT_KEY:
    return event->value ? ET_KEY_PRESSED : ET_KEY_RELEASED;
  default:
    NOTIMPLEMENTED();
    return ET_UNKNOWN;
  }
  return ET_UNKNOWN;
}

int EventFlagsFromNative(const base::NativeEvent& native_event) {
  return native_event->modifiers;
}

base::TimeDelta EventTimeFromNative(const base::NativeEvent& native_event) {
  //TODO(msb): implement this
  return base::TimeDelta();
}

gfx::Point EventLocationFromNative(const base::NativeEvent& native_event) {
  return gfx::Point(native_event->x, native_event->y);
}

KeyboardCode KeyboardCodeFromNative(const base::NativeEvent& native_event) {
  return ui::KeyboardCodeFromXKeysym(native_event->code);
}

bool IsMouseEvent(const base::NativeEvent& event) {
  switch (event->type) {
  case base::EVDEV_EVENT_RELATIVE_MOTION:
    return true;
  default:
    return false;
  }
  return false;
}

int GetMouseWheelOffset(const base::NativeEvent& native_event) {
  NOTIMPLEMENTED();
  return 0;
}

bool GetScrollOffsets(const base::NativeEvent& native_event,
                      float* x_offset,
                      float* y_offset) {
  return false;
}

int GetTouchId(const base::NativeEvent& native_event) {
  NOTIMPLEMENTED();
  return 0;
}

float GetTouchRadiusX(const base::NativeEvent& native_event) {
  NOTIMPLEMENTED();
  return 0.0;
}

float GetTouchRadiusY(const base::NativeEvent& native_event) {
  NOTIMPLEMENTED();
  return 0.0;
}

float GetTouchAngle(const base::NativeEvent& native_event) {
  NOTIMPLEMENTED();
  return 0.0;
}

float GetTouchForce(const base::NativeEvent& native_event) {
  NOTIMPLEMENTED();
  return 0.0;
}

base::NativeEvent CreateNoopEvent() {
  return (base::NativeEvent)NULL;
}

bool IsNoopEvent(const base::NativeEvent& native_event) {
  if (native_event == (base::NativeEvent)NULL)
    return true;
  return false;
}

}  // namespace ui
