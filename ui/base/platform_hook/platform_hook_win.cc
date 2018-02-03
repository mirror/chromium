// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/platform_hook/platform_hook.h"

#include <memory>
#include <utility>

#include <windows.h>

#include "base/callback.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"

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
  explicit PlatformHookWin(OnKeyEventCallback callback);
  ~PlatformHookWin() override;

  // PlatformHook interface.
  bool Register() override;
  bool Unregister() override;

 private:
  static LRESULT ProcessKeyEvent(int code, WPARAM w_param, LPARAM l_param);
  static PlatformHookWin* instance_;

  THREAD_CHECKER(thread_checker_);

  OnKeyEventCallback on_key_event_;
  HHOOK hook_ = NULL;

  DISALLOW_COPY_AND_ASSIGN(PlatformHookWin);
};

// static
std::unique_ptr<PlatformHook> PlatformHook::Create(
    OnKeyEventCallback callback) {
  return std::make_unique<PlatformHookWin>(callback);
}

// static
PlatformHookWin* PlatformHookWin::instance_ = nullptr;

PlatformHookWin::PlatformHookWin(OnKeyEventCallback callback)
    : on_key_event_(callback) {
  DCHECK(on_key_event_);
}

PlatformHookWin::~PlatformHookWin() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  Unregister();
}

bool PlatformHookWin::Register() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Only one instance of this class can be registered at a time.  This method
  // could be called multiple times (to reserve a different set of keys) so
  // we want to allow that condition.
  DCHECK(!instance_ || instance_ == this);
  instance_ = this;

  // TODO(joedow): Update key filter map.

  if (!hook_) {
    // Per MSDN this Hook procedure will be called in the context of the thread
    // which installed it.
    hook_ = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        reinterpret_cast<HOOKPROC>(&PlatformHookWin::ProcessKeyEvent),
        /*hMod=*/NULL,
        /*dwThreadId=*/0);
    PLOG_IF(ERROR, !hook_) << "SetWindowsHookEx failed";
  }

  return hook_ != NULL;
}

bool PlatformHookWin::Unregister() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (hook_) {
    // This instance should own the hook if it is set.
    DCHECK_EQ(instance_, this);
    if (!UnhookWindowsHookEx(hook_)) {
      PLOG(ERROR) << "UnhookWindowsHookEx failed";
    }
    hook_ = NULL;
    instance_ = nullptr;
  }

  return hook_ == NULL;
}

// static
LRESULT PlatformHookWin::ProcessKeyEvent(int code,
                                         WPARAM w_param,
                                         LPARAM l_param) {
  // If there is an error unhooking, this method could be called with a null
  // |instance_|.  Check for that before proceeding.
  if (!instance_) {
    return CallNextHookEx(nullptr, code, w_param, l_param);
  }

  DCHECK_CALLED_ON_VALID_THREAD(instance_->thread_checker_);
  DCHECK(instance_->on_key_event_);

  KBDLLHOOKSTRUCT* ll_hooks = reinterpret_cast<KBDLLHOOKSTRUCT*>(l_param);
  if (code == HC_ACTION && !IsOSReservedKey(ll_hooks->vkCode)) {
    MSG msg = {nullptr, w_param, ll_hooks->vkCode,
               (ll_hooks->scanCode << 16) | (ll_hooks->flags & 0xFFFF),
               ll_hooks->time};
    ui::KeyEvent event(msg);
    event.set_flags(event.flags() | ui::EF_FROM_PLATFORM_HOOK);
    instance_->on_key_event_.Run(&event);
    return 1;
  }
  return CallNextHookEx(nullptr, code, w_param, l_param);
}

}  // namespace views
