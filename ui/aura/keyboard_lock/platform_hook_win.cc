// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/keyboard_lock/platform_hook_win.h"

#include <windows.h>

#include "base/logging.h"
#include "base/threading/platform_thread.h"
#include "ui/events/platform/platform_key_event_filter.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

// static
PlatformHookWin* PlatformHookWin::me = nullptr;

PlatformHookWin::PlatformHookWin(KeyEventFilter* filter)
    : PlatformHook(filter),
      platform_key_event_filter_(filter) {
  DCHECK(me == nullptr);
  me = this;
}

PlatformHookWin::~PlatformHookWin() {
  me = nullptr;
}

void PlatformHookWin::Register(const std::vector<NativeKeyCode>& codes,
                               base::Callback<void(bool)> on_result) {
  LOG(ERROR) << "Will register " << codes.size() << " keys";
  registered_keys_ += static_cast<int>(codes.size());
  LOG(ERROR) << "New registered_keys " << registered_keys_;
  if (registered_keys_ == static_cast<int>(codes.size())) {
    hook_ = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        static_cast<HOOKPROC>(&PlatformHookWin::ProcessKeyEvent),
        NULL,
        0); // base::PlatformThread::CurrentId());
    LOG(ERROR) << "SetWindowsHookEx returns " << (hook_ != NULL);
    if (on_result) {
      on_result.Run(hook_ != NULL);
    }
    return;
  }

  if (on_result) {
    on_result.Run(true);
  }
}

void PlatformHookWin::Unregister(const std::vector<NativeKeyCode>& codes,
                                 base::Callback<void(bool)> on_result) {
  LOG(ERROR) << "Will unregister " << codes.size() << " keys";
  registered_keys_ -= static_cast<int>(codes.size());
  DCHECK_GE(registered_keys_, 0);
  LOG(ERROR) << "New registered_keys " << registered_keys_;
  if (registered_keys_ == 0) {
    if (hook_) {
      const bool result = (UnhookWindowsHookEx(hook_) != 0);
      LOG(ERROR) << "UnhookWindowsHookEx returns " << result;
      if (on_result) {
        on_result.Run(result);
      }
      return;
    }
  }

  if (on_result) {
    on_result.Run(true);
  }
}

// static
LRESULT PlatformHookWin::ProcessKeyEvent(int code,
                                         WPARAM w_param,
                                         LPARAM l_param) {
  DCHECK(me);
  KBDLLHOOKSTRUCT* ll_hooks = reinterpret_cast<KBDLLHOOKSTRUCT*>(l_param);
  MSG msg = {
    nullptr,
    w_param,
    ll_hooks->vkCode,
    (ll_hooks->scanCode << 16) | (ll_hooks->flags & 0xFFFF),
    ll_hooks->time
  };
  LOG(ERROR) << "Receiving key " << ll_hooks->vkCode;
  if (code == 0 && me->platform_key_event_filter_.OnPlatformEvent(msg)) {
    return 1;
  }
  return CallNextHookEx(nullptr, code, w_param, l_param);
}

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui
