// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include <windows.h>

#include "base/logging.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "ui/base/keyboard_lock/platform_hook.h"
#include "ui/events/platform/key_event_filter.h"
#include "ui/events/platform/platform_key_event_filter.h"

namespace ui {

namespace {

// These keys interfere with the return value of ::GetKeyState() as observing
// them directly in LowLevelKeyboardProc will cause their key combinations to be
// ignored.  As an example, the KeyF event in an Alt + F combination will result
// in a missing alt-down flag. Since a regular application can successfully
// receive these keys without using LowLevelKeyboardProc, they can be ignored.
bool IsOSReservedKey(DWORD vk) {
  return vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT ||
         vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ||
         vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU || vk == VK_LWIN ||
         vk == VK_RWIN || vk == VK_CAPITAL || vk == VK_NUMLOCK ||
         vk == VK_SCROLL;
}

}  // namespace

class PlatformHookWin : public PlatformHook {
 public:
  explicit PlatformHookWin(KeyEventFilter* filter);
  ~PlatformHookWin() override;

  // PlatformHook interface.
  void Register(base::OnceCallback<void(bool)> done) override;
  void Unregister(base::OnceCallback<void(bool)> done) override;

 private:
  static LRESULT ProcessKeyEvent(int code, WPARAM w_param, LPARAM l_param);
  static PlatformHookWin* instance_;

  THREAD_CHECKER(thread_checker_);

  PlatformKeyEventFilter platform_key_event_filter_;
  HHOOK hook_ = NULL;

  DISALLOW_COPY_AND_ASSIGN(PlatformHookWin);
};

// static
std::unique_ptr<PlatformHook> PlatformHook::Create(KeyEventFilter* filter) {
  return std::make_unique<PlatformHookWin>(filter);
}

// static
PlatformHookWin* PlatformHookWin::instance_ = nullptr;

PlatformHookWin::PlatformHookWin(KeyEventFilter* filter)
    : platform_key_event_filter_(filter) {
  // Only one instance of this class can exist at a time.  There should only be
  // one frame active and focused at a time so having this constraint should
  // help prevent odd situations where multiple hooks are registered.
  void* old_instance = InterlockedExchangePointer(
      reinterpret_cast<void* volatile*>(&instance_), this);
  DCHECK(!old_instance);
}

PlatformHookWin::~PlatformHookWin() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  Unregister(base::OnceCallback<void(bool)>());

  InterlockedExchangePointer(reinterpret_cast<void* volatile*>(&instance_),
                             nullptr);
}

void PlatformHookWin::Register(base::OnceCallback<void(bool)> done) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!hook_) {
    // Per MSDN this Hook Procedure will be called in the context of the thread
    // which installed it.
    hook_ = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        reinterpret_cast<HOOKPROC>(&PlatformHookWin::ProcessKeyEvent),
        /*hMod=*/NULL,
        /*dwThreadId =*/0);
    PLOG_IF(ERROR, !hook_) << "SetWindowsHookEx failed";
  }

  if (done) {
    std::move(done).Run(hook_ != NULL);
  }
}

void PlatformHookWin::Unregister(base::OnceCallback<void(bool)> done) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (hook_) {
    if (UnhookWindowsHookEx(hook_)) {
      hook_ = NULL;
    } else {
      PLOG(ERROR) << "UnhookWindowsHookEx failed";
    }
  }

  if (done) {
    std::move(done).Run(hook_ == NULL);
  }
}

// static
LRESULT PlatformHookWin::ProcessKeyEvent(int code,
                                         WPARAM w_param,
                                         LPARAM l_param) {
  DCHECK(instance_);
  KBDLLHOOKSTRUCT* ll_hooks = reinterpret_cast<KBDLLHOOKSTRUCT*>(l_param);
  if (code == HC_ACTION && !IsOSReservedKey(ll_hooks->vkCode)) {
    MSG msg = {
      nullptr,
      w_param,
      ll_hooks->vkCode,
      (ll_hooks->scanCode << 16) | (ll_hooks->flags & 0xFFFF),
      ll_hooks->time
    };
    if (instance_->platform_key_event_filter_.OnPlatformEvent(msg)) {
      return 1;
    }
  }
  return CallNextHookEx(nullptr, code, w_param, l_param);
}

}  // namespace ui
