// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_H_
#define COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_H_

#include <vector>

#include "base/callback.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace keyboard_lock {

// An interface to register and unregister key codes from low level callback.
// The implementation needs to guarantee the thread-safety of RegisterKey() and
// UnregisterKey() functions, which may be executed in both UI thread and
// network thread.
class KeyHook {
 public:
  KeyHook() = default;
  virtual ~KeyHook() = default;

  // The |on_result| callback will be called with the result if it's not null.
  // Calling |on_result| with |false| indicates the failure of an OK API
  // invoking. Thus, the required ui::KeyboardCode may not be able to received.
  virtual void RegisterKey(const std::vector<ui::KeyboardCode>& codes,
                           base::Callback<void(bool)> on_result) = 0;
  virtual void UnregisterKey(const std::vector<ui::KeyboardCode>& codes,
                             base::Callback<void(bool)> on_result) = 0;
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_H_
