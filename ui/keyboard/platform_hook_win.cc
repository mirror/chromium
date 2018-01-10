// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <vector>

#include <windows.h>

#include "base/logging.h"
#include "base/threading/platform_thread.h"
#include "ui/events/platform/platform_key_event_filter.h"
#include "ui/keyboard/platform_hook.h"

namespace ui {
namespace keyboard {

namespace {

// These keys impact the return value of the ::GetKeyState() function, consuming
// them directly in LowLevelKeyboardProc will cause their key combinations to be
// ignored: i.e. the KeyF event in an Alt + F combination will result in a
// missing alt-down flag. Since a regular application can successfully receive
// these keys without using LowLevelKeyboardProc, we can safely ignore them.
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


// TODO: THIS CLASS IS NOT THREAD SAFE AS RELYING ON A STATIC INSTANCE* WITH A
// DCHECK IS OPEN TO RACE CONDITIONS BETWEEN THREADS.  FIX THIS BEFORE CR.
class PlatformHookWin : public PlatformHook {
 public:
  explicit PlatformHookWin(KeyEventFilter* filter);
  ~PlatformHookWin() override;

  // PlatformHook interface.
  void Register(base::OnceCallback<void(bool)> done) override;
  void Unregister(base::OnceCallback<void(bool)> done) override;

 private:
  static LRESULT ProcessKeyEvent(int code, WPARAM w_param, LPARAM l_param);

  static PlatformHookWin* instance;
  PlatformKeyEventFilter platform_key_event_filter_;
  HHOOK hook_ = nullptr;
};

// static
std::unique_ptr<PlatformHook> PlatformHook::Create(KeyEventFilter* filter) {
  return std::make_unique<PlatformHookWin>(filter);
}

// static
PlatformHookWin* PlatformHookWin::instance = nullptr;

PlatformHookWin::PlatformHookWin(KeyEventFilter* filter)
    : platform_key_event_filter_(filter) {
  // Only one instance of this class can exist at a time.
  DCHECK(!instance);
  instance = this;
}

PlatformHookWin::~PlatformHookWin() {
  instance = nullptr;
}

void PlatformHookWin::Register(base::OnceCallback<void(bool)> done) {
  if (!hook_) {
    // TODO(joedow): Determine if we need to set thread affinity via dwThreadId.
    hook_ = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        reinterpret_cast<HOOKPROC>(&PlatformHookWin::ProcessKeyEvent),
        /*hMod=*/ NULL,
        /*dwThreadId =*/ 0);
    PLOG_IF(ERROR, !hook_) << "SetWindowsHookEx failed";
  }

  if (done) {
    std::move(done).Run(hook_ != NULL);
  }
}

void PlatformHookWin::Unregister(base::OnceCallback<void(bool)> done) {
  if (hook_) {
    bool result = (UnhookWindowsHookEx(hook_) != 0);
    PLOG_IF(ERROR, !result) << "UnhookWindowsHookEx failed";
    hook_ = NULL;
  }

  if (done) {
    std::move(done).Run(!hook_);
  }
}

// static
LRESULT PlatformHookWin::ProcessKeyEvent(int code,
                                         WPARAM w_param,
                                         LPARAM l_param) {
  DCHECK(instance);
  KBDLLHOOKSTRUCT* ll_hooks = reinterpret_cast<KBDLLHOOKSTRUCT*>(l_param);
  if (code == 0 && !IsOSReservedKey(ll_hooks->vkCode)) {
    MSG msg = {
      nullptr,
      w_param,
      ll_hooks->vkCode,
      (ll_hooks->scanCode << 16) | (ll_hooks->flags & 0xFFFF),
      ll_hooks->time
    };
    if (instance->platform_key_event_filter_.OnPlatformEvent(msg)) {
      return 1;
    }
  }
  return CallNextHookEx(nullptr, code, w_param, l_param);
}

}  // namespace keyboard
}  // namespace ui
