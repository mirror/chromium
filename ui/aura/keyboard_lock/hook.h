// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_KEYBOARD_LOCK_HOOK_H_
#define UI_AURA_KEYBOARD_LOCK_HOOK_H_

#include <vector>

#include "base/callback.h"
#include "ui/aura/keyboard_lock/types.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

// A Hook is an interface to allow other components to register a set of keys
// to.
class Hook {
 public:
  Hook() = default;
  virtual ~Hook() = default;

  // Registers a set of NativeKeyCodes. |on_result| will be called with the
  // result of the registration if it's not empty.
  virtual void Register(const std::vector<NativeKeyCode>& codes,
                        base::Callback<void(bool)> on_result) = 0;
  // Unregisters a set of NativeKeyCodes. |on_result| will be called with the
  // result of the registration if it's not empty.
  virtual void Unregister(const std::vector<NativeKeyCode>& codes,
                          base::Callback<void(bool)> on_result) = 0;
};

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui

#endif  // UI_AURA_KEYBOARD_LOCK_HOOK_H_
