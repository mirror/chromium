// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_PLATFORM_KEY_HOOK_H_
#define COMPONENTS_KEYBOARD_LOCK_PLATFORM_KEY_HOOK_H_

#include <memory>

#include "components/keyboard_lock/platform_key_event_filter.h"
#include "components/keyboard_lock/sync_key_hook.h"

class Browser;

namespace keyboard_lock {

class KeyEventFilter;

class PlatformKeyHook final : public SyncKeyHook {
 public:
  PlatformKeyHook(Browser* owner, KeyEventFilter* filter);
  ~PlatformKeyHook() override;

 private:
  // SyncKeyHook implementations
  bool RegisterKey(const std::vector<ui::KeyboardCode>& codes) override;
  bool UnregisterKey(const std::vector<ui::KeyboardCode>& codes) override;

  Browser* const owner_;
  PlatformKeyEventFilter filter_;
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_PLATFORM_KEY_HOOK_H_
