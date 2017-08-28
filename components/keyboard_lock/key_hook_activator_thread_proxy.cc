// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "components/keyboard_lock/key_hook_activator_thread_proxy.h"

namespace keyboard_lock {

KeyHookActivatorThreadProxy::KeyHookActivatorThreadProxy(
    KeyHookActivator* const key_hook,
    scoped_refptr<base::SingleThreadTaskRunner> runner)
    : key_hook_(key_hook),
      runner_(std::move(runner)) {
  DCHECK(key_hook_);
  DCHECK(runner_);
}

KeyHookActivatorThreadProxy::~KeyHookActivatorThreadProxy() = default;

void KeyHookActivatorThreadProxy::RegisterKey(
    const std::vector<ui::KeyboardCode>& codes,
    base::Callback<void(bool)> on_result) {
  runner_->PostTask(FROM_HERE,
      base::Bind([](KeyHookActivator* const key_hook,
                    const std::vector<ui::KeyboardCode>& codes,
                    base::Callback<void(bool)> on_result) {
        key_hook->RegisterKey(codes, std::move(on_result));
      },
      base::Unretained(key_hook_),
      codes,
      std::move(on_result)));
}

void KeyHookActivatorThreadProxy::UnregisterKey(
    const std::vector<ui::KeyboardCode>& codes,
    base::Callback<void(bool)> on_result) {
  runner_->PostTask(FROM_HERE,
      base::Bind([](KeyHookActivator* const key_hook,
                    const std::vector<ui::KeyboardCode>& codes,
                    base::Callback<void(bool)> on_result) {
        key_hook->UnregisterKey(codes, std::move(on_result));
      },
      base::Unretained(key_hook_),
      codes,
      std::move(on_result)));
}

void KeyHookActivatorThreadProxy::Activate(
    base::Callback<void(bool)> on_result) {
  runner_->PostTask(FROM_HERE,
      base::Bind([](KeyHookActivator* const key_hook,
                    base::Callback<void(bool)> on_result) {
        key_hook->Activate(std::move(on_result));
      },
      base::Unretained(key_hook_),
      std::move(on_result)));
}

void KeyHookActivatorThreadProxy::Deactivate(
    base::Callback<void(bool)> on_result) {
  runner_->PostTask(FROM_HERE,
      base::Bind([](KeyHookActivator* const key_hook,
                    base::Callback<void(bool)> on_result) {
        key_hook->Deactivate(std::move(on_result));
      },
      base::Unretained(key_hook_),
      std::move(on_result)));
}

bool KeyHookActivatorThreadProxy::IsKeyReserved(ui::KeyboardCode code) const {
  return key_hook_->IsKeyReserved(code);
}

}  // namespace keyboard_lock
