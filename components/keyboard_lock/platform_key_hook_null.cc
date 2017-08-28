// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/platform_key_hook.h"

namespace keyboard_lock {

PlatformKeyHook::PlatformKeyHook(Browser* owner, KeyEventFilter* filter)
    : owner_(owner),
      filter_(filter) {}

PlatformKeyHook::~PlatformKeyHook() = default;

bool PlatformKeyHook::RegisterKey(const std::vector<ui::KeyboardCode>& codes) {
  return false;
}

bool PlatformKeyHook::UnregisterKey(
    const std::vector<ui::KeyboardCode>& codes) {
  return false;
}

}  // namespace keyboard_lock
