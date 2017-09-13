// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_KEYBOARD_LOCK_PLATFORM_HOOK_NULL_H_
#define UI_AURA_KEYBOARD_LOCK_PLATFORM_HOOK_NULL_H_

#include "ui/aura/keyboard_lock/platform_hook.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

class PlatformHookNull : public PlatformHook {
 public:
  PlatformHookNull(KeyEventFilter* filter);
  ~PlatformHookNull() override;

  void Register(const std::vector<NativeKeyCode>& codes,
                base::Callback<void(bool)> on_result) override;
  void Unregister(const std::vector<NativeKeyCode>& codes,
                  base::Callback<void(bool)> on_result) override;
};

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui

#endif  // UI_AURA_KEYBOARD_LOCK_PLATFORM_HOOK_NULL_H_
