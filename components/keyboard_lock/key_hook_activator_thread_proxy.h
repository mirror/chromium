// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_ACTIVITOR_THREAD_PROXY_H_
#define COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_ACTIVITOR_THREAD_PROXY_H_

#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "components/keyboard_lock/key_hook_activator.h"

namespace keyboard_lock {

// An implementation of KeyHookActivator to forward requests to |runner|.
class KeyHookActivatorThreadProxy final : public KeyHookActivator {
 public:
  // |key_hook| should never be destroyed.
  KeyHookActivatorThreadProxy(
      KeyHookActivator* const key_hook,
      scoped_refptr<base::SingleThreadTaskRunner> runner);
  ~KeyHookActivatorThreadProxy() override;

  // KeyHookActivator implementations.
  void RegisterKey(const std::vector<ui::KeyboardCode>& codes,
                   base::Callback<void(bool)> on_result) override;
  void UnregisterKey(const std::vector<ui::KeyboardCode>& codes,
                     base::Callback<void(bool)> on_result) override;
  void Activate(base::Callback<void(bool)> on_result) override;
  void Deactivate(base::Callback<void(bool)> on_result) override;

 private:
  KeyHookActivator* const key_hook_;
  const scoped_refptr<base::SingleThreadTaskRunner> runner_;
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_KEY_HOOK_ACTIVITOR_THREAD_PROXY_H_
