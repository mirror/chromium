// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_SHARE_WRAPPER_H_
#define COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_SHARE_WRAPPER_H_

#include "components/keyboard_lock/key_hook.h"

namespace keyboard_lock {

class KeyHookShareWrapper final : public KeyHook {
 public:
  KeyHookShareWrapper(KeyHook* key_hook);
  ~KeyHookShareWrapper() override;

 private:
  // KeyHook implementations
  void RegisterKey(const std::vector<ui::KeyboardCode>& codes,
                   base::Callback<void(bool)> on_result) override;
  void UnregisterKey(const std::vector<ui::KeyboardCode>& codes,
                     base::Callback<void(bool)> on_result) override;

  KeyHook* const key_hook_;
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_SHARE_WRAPPER_H_
