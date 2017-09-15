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

namespace {

// These keys impact the return value of the ::GetKeyState() function, consuming
// them directly in LowLevelKeyboardProc will cause the their key combinations
// to be ignored. I.e. the event of the KeyF in Alt + F combination will have no
// alt-down flag. Since a regular application can successfully receive these
// keys even without LowLevelKeyboardProc, we can simply ignore them to avoid
// introducing any trouble.
bool IsOSReservedKey(DWORD vk) {
  return vk == VK_SHIFT ||
         vk == VK_LSHIFT ||
         vk == VK_RSHIFT ||
         vk == VK_CONTROL ||
         vk == VK_LCONTROL ||
         vk == VK_RCONTROL ||
         vk == VK_MENU ||
         vk == VK_LMENU ||
         vk == VK_RMENU ||
         vk == VK_LWIN ||
         vk == VK_RWIN ||
         vk == VK_CAPITAL ||
         vk == VK_NUMLOCK ||
         vk == VK_SCROLL;
}

}  // namespace

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
  registered_keys_ += static_cast<int>(codes.size());
  if (registered_keys_ == static_cast<int>(codes.size())) {
    hook_ = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        reinterpret_cast<HOOKPROC>(&PlatformHookWin::ProcessKeyEvent),
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
  registered_keys_ -= static_cast<int>(codes.size());
  DCHECK_GE(registered_keys_, 0);
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
  if (code == 0 && !IsOSReservedKey(ll_hooks->vkCode)) {
    MSG msg = {
      nullptr,
      w_param,
      ll_hooks->vkCode,
      (ll_hooks->scanCode << 16) | (ll_hooks->flags & 0xFFFF),
      ll_hooks->time
    };
    if (me->platform_key_event_filter_.OnPlatformEvent(msg)) {
      return 1;
    }
  }
  return CallNextHookEx(nullptr, code, w_param, l_param);
}

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui
