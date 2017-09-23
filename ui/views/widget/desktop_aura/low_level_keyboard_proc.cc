// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/widget/desktop_aura/low_level_keyboard_proc.h"

#include "base/memory/singleton.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/views/win/hwnd_message_handler_delegate.h"

namespace views {

// static
LowLevelKeyboardProc* LowLevelKeyboardProc::GetInstance() {
  return base::Singleton<LowLevelKeyboardProc>::get();
}

LowLevelKeyboardProc::LowLevelKeyboardProc() = default;
LowLevelKeyboardProc::~LowLevelKeyboardProc() = default;

bool LowLevelKeyboardProc::LockKeys(HWNDMessageHandlerDelegate* callback) {
  callback_ = callback;
  if (hook_ == NULL) {
    hook_ = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        reinterpret_cast<HOOKPROC>(&LowLevelKeyboardProc::ProcessCallback),
        NULL,
        0);
    return hook_ != NULL;
  }
  return true;
}

bool LowLevelKeyboardProc::UnlockKeys(HWNDMessageHandlerDelegate* callback) {
  if (callback != callback_) {
    return true;
  }

  if (hook_ == NULL) {
    return true;
  }

  return UnhookWindowsHookEx(hook_) != 0;
}

// static
LRESULT LowLevelKeyboardProc::ProcessCallback(int code,
                                              WPARAM w_param,
                                              LPARAM l_param) {
  if (code == 0) {
    KBDLLHOOKSTRUCT* ll_hooks = reinterpret_cast<KBDLLHOOKSTRUCT*>(l_param);
    if (GetInstance()->ProcessKeyEvent(w_param, ll_hooks)) {
      return 1;
    }
  }
  return CallNextHookEx(nullptr, code, w_param, l_param);
}

bool LowLevelKeyboardProc::ProcessKeyEvent(WPARAM w_param,
                                           KBDLLHOOKSTRUCT* ll_hooks) {
  if (!callback_) {
    return false;
  }

  MSG msg = {
    nullptr,
    w_param,
    ll_hooks->vkCode,
    (ll_hooks->scanCode << 16) | (ll_hooks->flags & 0xFFFF),
    ll_hooks->time
  };
  ui::KeyEvent event(msg);
  event.set_flags(event.flags() | ui::EF_RESERVED);
  callback_->HandleKeyEvent(&event);
  return event.handled() || true;
}

}  // namespace views
