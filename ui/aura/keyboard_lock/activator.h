// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_KEYBOARD_LOCK_ACTIVATOR_H_
#define UI_AURA_KEYBOARD_LOCK_ACTIVATOR_H_

#include "base/callback.h"
#include "ui/aura/keyboard_lock/hook.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

// An interface to fast enable or disable all registered keys of a Hook.
class Activator : public Hook {
 public:
  Activator() = default;
  ~Activator() override = default;

  // Enables all registered keys. This function does not change the registered
  // keys, but only forward them to the underlying components.
  virtual void Activate(base::Callback<void(bool)> on_result) = 0;

  // Disables all registered keys. This function does not change the registered
  // keys, but only forward them to the underlying components.
  virtual void Deactivate(base::Callback<void(bool)> on_result) = 0;
};

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui

#endif  // UI_AURA_KEYBOARD_LOCK_ACTIVATOR_H_
