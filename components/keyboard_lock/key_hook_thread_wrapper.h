// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_THREAD_WRAPPER_H_
#define COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_THREAD_WRAPPER_H_

#include <memory>

#include "base/synchronization/lock.h"
#include "components/keyboard_lock/key_event_filter.h"
#include "components/keyboard_lock/key_hook_activator.h"

namespace keyboard_lock {

class KeyHookStateKeeper;

// An implementation of both KeyHookActivator and KeyEventFilter to ensure all
// the public functions are not called parallely from multiple threads.
class KeyHookThreadWrapper final
    : public KeyEventFilter,
      public KeyHookActivator {
 public:
  // Takes ownership of both objects.
  KeyHookThreadWrapper(std::unique_ptr<KeyEventFilter> filter,
                       std::unique_ptr<KeyHookActivator> key_hook);
  // A shortcut to create a KeyHookThreadWrapper from a KeyHookStateKeeper,
  // which is the typical usage of this class.
  explicit KeyHookThreadWrapper(
      std::unique_ptr<KeyHookStateKeeper> state_keeper);
  ~KeyHookThreadWrapper() override;

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
  const std::unique_ptr<KeyHookActivator> key_hook_;
  base::Lock lock_;
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_THREAD_WRAPPER_H_
