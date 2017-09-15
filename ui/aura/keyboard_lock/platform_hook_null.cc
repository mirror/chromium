// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/keyboard_lock/platform_hook_null.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

PlatformHookNull::PlatformHookNull(KeyEventFilter* filter)
    : PlatformHook(filter) {}
PlatformHookNull::~PlatformHookNull() = default;

void PlatformHookNull::Register(const std::vector<NativeKeyCode>& codes,
                                base::Callback<void(bool)> on_result) {
  if (on_result) {
    on_result.Run(false);
  }
}

void PlatformHookNull::Unregister(const std::vector<NativeKeyCode>& codes,
                                  base::Callback<void(bool)> on_result) {
  if (on_result) {
    on_result.Run(false);
  }
}

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui
