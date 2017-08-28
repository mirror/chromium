// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_SYNC_KEY_HOOK_H_
#define COMPONENTS_KEYBOARD_LOCK_SYNC_KEY_HOOK_H_

#include "components/keyboard_lock/key_hook.h"

namespace keyboard_lock {

// An implementation of KeyHook for synchronized implementation, i.e. both
// RegisterKey() and UnregisterKey() return synchronizedly. Usually a platform
// specific implementation should derived from this class.
class SyncKeyHook : public KeyHook {
 public:
  SyncKeyHook();
  ~SyncKeyHook() override;

  void RegisterKey(const std::vector<ui::KeyboardCode>& codes,
                   base::Callback<void(bool)> on_result) final;
  void UnregisterKey(const std::vector<ui::KeyboardCode>& codes,
                     base::Callback<void(bool)> on_result) final;

  virtual bool RegisterKey(const std::vector<ui::KeyboardCode>& codes) = 0;
  virtual bool UnregisterKey(const std::vector<ui::KeyboardCode>& codes) = 0;
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_SYNC_KEY_HOOK_H_
