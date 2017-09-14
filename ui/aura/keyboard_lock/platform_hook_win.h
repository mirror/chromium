// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_KEYBOARD_LOCK_PLATFORM_HOOK_WIN_H_
#define UI_AURA_KEYBOARD_LOCK_PLATFORM_HOOK_WIN_H_

#include "ui/aura/keyboard_lock/platform_hook.h"

#include "ui/events/platform/platform_key_event_filter.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

class PlatformHookWin : public PlatformHook {
 public:
  PlatformHookWin(KeyEventFilter* filter);
  ~PlatformHookWin() override;

  void Register(const std::vector<NativeKeyCode>& codes,
                base::Callback<void(bool)> on_result) override;
  void Unregister(const std::vector<NativeKeyCode>& codes,
                  base::Callback<void(bool)> on_result) override;

 private:
  static LRESULT ProcessKeyEvent(int code, WPARAM w_param, LPARAM l_param);

  static PlatformHookWin* me;
  int registered_keys_ = 0;
  PlatformKeyEventFilter platform_key_event_filter_;
  HHOOK hook_ = nullptr;
};

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui

#endif  // UI_AURA_KEYBOARD_LOCK_PLATFORM_HOOK_WIN_H_
