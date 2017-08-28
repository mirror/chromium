// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/key_hook_thread_wrapper.h"

#include <memory>
#include <utility>

#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "components/keyboard_lock/key_event_filter_share_wrapper.h"
#include "components/keyboard_lock/key_hook_state_keeper.h"

namespace keyboard_lock {

KeyHookThreadWrapper::KeyHookThreadWrapper(
    std::unique_ptr<KeyEventFilter> filter,
    std::unique_ptr<KeyHookActivator> key_hook)
    : filter_(std::move(filter)),
      key_hook_(std::move(key_hook)) {
  DCHECK(filter_);
  DCHECK(key_hook_);
}

KeyHookThreadWrapper::KeyHookThreadWrapper(
    std::unique_ptr<KeyHookStateKeeper> state_keeper)
    : KeyHookThreadWrapper(
          base::MakeUnique<KeyEventFilterShareWrapper>(state_keeper.get()),
          std::move(state_keeper)) {}

KeyHookThreadWrapper::~KeyHookThreadWrapper() = default;

bool KeyHookThreadWrapper::OnKeyDown(ui::KeyboardCode code, int flags) {
  base::AutoLock lock(lock_);
  return filter_->OnKeyDown(code, flags);
}

bool KeyHookThreadWrapper::OnKeyUp(ui::KeyboardCode code, int flags) {
  base::AutoLock lock(lock_);
  return filter_->OnKeyUp(code, flags);
}

void KeyHookThreadWrapper::RegisterKey(
    const std::vector<ui::KeyboardCode>& codes,
    base::Callback<void(bool)> on_result) {
  base::AutoLock lock(lock_);
  key_hook_->RegisterKey(codes, on_result);
}

void KeyHookThreadWrapper::UnregisterKey(
    const std::vector<ui::KeyboardCode>& codes,
    base::Callback<void(bool)> on_result) {
  base::AutoLock lock(lock_);
  key_hook_->UnregisterKey(codes, on_result);
}

void KeyHookThreadWrapper::Activate(
    base::Callback<void(bool)> on_result) {
  base::AutoLock lock(lock_);
  key_hook_->Activate(on_result);
}

void KeyHookThreadWrapper::Deactivate(
    base::Callback<void(bool)> on_result) {
  base::AutoLock lock(lock_);
  key_hook_->Deactivate(on_result);
}

bool KeyHookThreadWrapper::IsKeyReserved(ui::KeyboardCode code) const {
  return key_hook_->IsKeyReserved(code);
}

}  // namespace keyboard_lock
