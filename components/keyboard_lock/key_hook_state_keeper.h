// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_STATE_KEEPER_H_
#define COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_STATE_KEEPER_H_

#include <memory>
#include <vector>

#include "components/keyboard_lock/key_event_filter.h"
#include "components/keyboard_lock/key_hook_activator.h"

namespace keyboard_lock {

// An implementation of both KeyHookActivator and KeyEventFilter by maintaining
// the internal states and filtering out unregistered key codes.
class KeyHookStateKeeper final
    : public KeyEventFilter,
      public KeyHookActivator {
 public:
  KeyHookStateKeeper(std::unique_ptr<KeyEventFilter> filter,
                     std::unique_ptr<KeyHook> key_hook);
  ~KeyHookStateKeeper() override;

  // KeyEventFilter implementations.
  bool OnKeyDown(ui::KeyboardCode code, int flags) override;
  bool OnKeyUp(ui::KeyboardCode code, int flags) override;

  // KeyHookActivator implementations.
  void RegisterKey(const std::vector<ui::KeyboardCode>& codes,
                   base::Callback<void(bool)> on_result) override;
  void UnregisterKey(const std::vector<ui::KeyboardCode>& codes,
                     base::Callback<void(bool)> on_result) override;
  void Activate(base::Callback<void(bool)> on_result) override;
  void Deactivate(base::Callback<void(bool)> on_result) override;
  bool IsKeyReserved(ui::KeyboardCode code) const override;

 private:
  const std::unique_ptr<KeyEventFilter> filter_;
  const std::unique_ptr<KeyHook> key_hook_;
  std::vector<bool> states_;
  bool active_;
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_STATE_KEEPER_H_
