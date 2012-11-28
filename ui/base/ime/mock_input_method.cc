// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/ime/mock_input_method.h"

#include "base/logging.h"
#include "base/string16.h"
#include "ui/base/events/event.h"
#include "ui/base/events/event_constants.h"
#include "ui/base/events/event_utils.h"
#include "ui/base/glib/glib_integers.h"
#include "ui/base/ime/input_method_delegate.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/base/keycodes/keyboard_code_conversion.h"

#if defined(USE_X11)
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "ui/base/keycodes/keyboard_code_conversion_x.h"
#endif

namespace {

#if defined(USE_X11)
uint32 EventFlagsFromXFlags(unsigned int flags) {
  return (flags & LockMask ? ui::EF_CAPS_LOCK_DOWN : 0U) |
      (flags & ControlMask ? ui::EF_CONTROL_DOWN : 0U) |
      (flags & ShiftMask ? ui::EF_SHIFT_DOWN : 0U) |
      (flags & Mod1Mask ? ui::EF_ALT_DOWN : 0U);
}
#endif

}  // namespace

namespace ui {

MockInputMethod::MockInputMethod(internal::InputMethodDelegate* delegate)
    : delegate_(NULL),
      text_input_client_(NULL) {
  SetDelegate(delegate);
}

MockInputMethod::~MockInputMethod() {
}

void MockInputMethod::SetDelegate(internal::InputMethodDelegate* delegate) {
  delegate_ = delegate;
}

void MockInputMethod::SetFocusedTextInputClient(TextInputClient* client) {
  text_input_client_ = client;
}

TextInputClient* MockInputMethod::GetTextInputClient() const {
  return text_input_client_;
}

void MockInputMethod::DispatchKeyEvent(const base::NativeEvent& native_event) {
#if defined(OS_WIN)
  if (native_event.message == WM_CHAR) {
    if (text_input_client_) {
      text_input_client_->InsertChar(ui::KeyboardCodeFromNative(native_event),
                                     ui::EventFlagsFromNative(native_event));
    }
  } else {
    delegate_->DispatchKeyEventPostIME(native_event);
  }
#elif defined(USE_X11) || defined(USE_EVDEV)
  DCHECK(native_event);
  if (ui::EventTypeFromNative(native_event) == ui::ET_KEY_RELEASED) {
    // On key release, just dispatch it.
    delegate_->DispatchKeyEventPostIME(native_event);
  } else {
    const uint32 state = EventFlagsFromNative(native_event);
    // Send a RawKeyDown event first,
    delegate_->DispatchKeyEventPostIME(native_event);
    if (text_input_client_) {
      // then send a Char event via ui::TextInputClient.
      const KeyboardCode key_code = ui::KeyboardCodeFromNative(native_event);
      uint16 ch = 0;
#if defined(USE_X11) // TODO(msb) do we need this for EVDEV
      if (!(state & ui::EF_CONTROL_DOWN))
        ch = ui::GetCharacterFromXEvent(native_event);
      if (!ch)
#endif
        ch = ui::GetCharacterFromKeyCode(key_code, state);
      if (ch)
        text_input_client_->InsertChar(ch, state);
    }
  }
#else
  // TODO(yusukes): Support other platforms. Call InsertChar() when necessary.
  delegate_->DispatchKeyEventPostIME(native_event);
  NOTIMPLEMENTED();
#endif
}

void MockInputMethod::DispatchFabricatedKeyEvent(const ui::KeyEvent& event) {
#if defined(OS_WIN)
  if (event.is_char()) {
    if (GetTextInputClient()) {
      GetTextInputClient()->InsertChar(event.key_code(),
                                       ui::GetModifiersFromKeyState());
      return;
    }
  }
  delegate_->DispatchFabricatedKeyEventPostIME(event.type(),
                                               event.key_code(),
                                               event.flags());
#endif
}

void MockInputMethod::Init(bool focused) {}
void MockInputMethod::OnFocus() {}
void MockInputMethod::OnBlur() {}
void MockInputMethod::OnTextInputTypeChanged(const TextInputClient* client) {}
void MockInputMethod::OnCaretBoundsChanged(const TextInputClient* client) {}
void MockInputMethod::CancelComposition(const TextInputClient* client) {}

std::string MockInputMethod::GetInputLocale() {
  return "";
}

base::i18n::TextDirection MockInputMethod::GetInputTextDirection() {
  return base::i18n::UNKNOWN_DIRECTION;
}

bool MockInputMethod::IsActive() {
  return true;
}

ui::TextInputType MockInputMethod::GetTextInputType() const {
  return ui::TEXT_INPUT_TYPE_NONE;
}

bool MockInputMethod::CanComposeInline() const {
  return true;
}

}  // namespace ui
