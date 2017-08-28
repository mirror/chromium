// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/platform_key_hook.h"

#include <windows.h>

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "components/keyboard_lock/register_once_key_hook.h"
#include "ui/events/platform/platform_event_types.h"

namespace {

class WinKeyHook final : public keyboard_lock::RegisterOnceKeyHook {
 public:
  WinKeyHook(base::Callback<bool(const ui::PlatformEvent&)>&& callback);
  ~WinKeyHook() = default;

  static WinKeyHook* instance();

 private:
  static LRESULT ProcessKeyEvent(int code, WPARAM w_param, LPARAM l_param);
  bool Register() override;
  bool Unregister() override;

  static WinKeyHook* me;
  const base::Callback<bool(const ui::PlatformEvent&)> callback_;
  HHOOK hook_ = nullptr;
};

// static
WinKeyHook* WinKeyHook::me = nullptr;

WinKeyHook::WinKeyHook(
    base::Callback<bool(const ui::PlatformEvent&)>&& callback)
    : callback_(std::move(callback)) {
  DCHECK(callback_);
  DCHECK(me == nullptr);
  me = this;
}

// static
WinKeyHook* WinKeyHook::instance() {
  return me;
}

bool WinKeyHook::Register() {
  LOG(ERROR) << "Going to SetWindowsHookEx";
  hook_ = SetWindowsHookEx(
      WH_KEYBOARD_LL,
      static_cast<HOOKPROC>(&WinKeyHook::ProcessKeyEvent),
      NULL,
      NULL);
  return hook_ != NULL;
}

bool WinKeyHook::Unregister() {
  LOG(ERROR) << "Going to UnhookWindowsHookEx";
  if (hook_) {
    return UnhookWindowsHookEx(hook_) != 0;
  }
  return true;
}

// static
LRESULT WinKeyHook::ProcessKeyEvent(int code, WPARAM w_param, LPARAM l_param) {
  DCHECK(me);
  KBDLLHOOKSTRUCT* ll_hooks = reinterpret_cast<KBDLLHOOKSTRUCT*>(l_param);
  MSG msg = { nullptr,
              w_param,
              ll_hooks->vkCode,
              (ll_hooks->scanCode << 16) | (ll_hooks->flags & 0xFFFF),
              ll_hooks->time };
  if (code == 0 && me->callback_.Run(msg)) {
    return 1;
  }
  return CallNextHookEx(nullptr, code, w_param, l_param);
}

}  // namespace

namespace keyboard_lock {

PlatformKeyHook::PlatformKeyHook(Browser* owner, KeyEventFilter* filter)
    : owner_(owner),
      filter_(filter) {
  if (!WinKeyHook::instance()) {
    new WinKeyHook(base::Bind(&PlatformKeyEventFilter::OnPlatformEvent,
                              base::Unretained(&filter_)));
  }
}

PlatformKeyHook::~PlatformKeyHook() = default;

bool PlatformKeyHook::RegisterKey(const std::vector<ui::KeyboardCode>& codes) {
  LOG(ERROR) << "Register " << codes.size() << " key codes";
  return WinKeyHook::instance()->RegisterKey(codes);
}

bool PlatformKeyHook::UnregisterKey(
    const std::vector<ui::KeyboardCode>& codes) {
  LOG(ERROR) << "Unregister " << codes.size() << " key codes";
  return WinKeyHook::instance()->UnregisterKey(codes);
}

}  // namespace keyboard_lock
