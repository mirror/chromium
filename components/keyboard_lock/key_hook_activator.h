// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_ACTIVATOR_H_
#define COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_ACTIVATOR_H_

#include <vector>

#include "base/callback.h"
#include "components/keyboard_lock/key_hook.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace keyboard_lock {

// An interface to store all registered key codes and register or unregister
// them with one Activate() or Deactivate() function call.
// The implementation needs to guarantee the thread-safety of Activate() and
// Deactivate() functions, which may be executed in both UI thread and
// network thread.
class KeyHookActivator : public KeyHook {
 public:
  KeyHookActivator() = default;
  ~KeyHookActivator() override = default;

  // Meaning of |on_result| is same as the one in KeyHook::RegisterKey().

  // Enables all registered key codes.
  virtual void Activate(base::Callback<void(bool)> on_result) = 0;

  // Disables all registered key codes.
  virtual void Deactivate(base::Callback<void(bool)> on_result) = 0;

  virtual bool IsKeyReserved(ui::KeyboardCode code) const = 0;
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_ACTIVATOR_H_
