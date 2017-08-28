// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTE_KEYBOARD_LOCK_REGISTER_ONCE_KEY_HOOK_H_
#define COMPONENTE_KEYBOARD_LOCK_REGISTER_ONCE_KEY_HOOK_H_

#include "components/keyboard_lock/sync_key_hook.h"

namespace keyboard_lock {

class RegisterOnceKeyHook : public SyncKeyHook {
 public:
  RegisterOnceKeyHook();
  ~RegisterOnceKeyHook() override;

  bool RegisterKey(const std::vector<ui::KeyboardCode>& codes) override;
  bool UnregisterKey(const std::vector<ui::KeyboardCode>& codes) override;

 private:
  virtual bool Register() = 0;
  virtual bool Unregister() = 0;

  int count_ = 0;
};

}  // namespace keyboard_lock

#endif  // COMPONENTE_KEYBOARD_LOCK_REGISTER_ONCE_KEY_HOOK_H_
