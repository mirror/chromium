// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/key_hook_share_wrapper.h"

#include <utility>

#include "base/logging.h"

namespace keyboard_lock {

KeyHookShareWrapper::KeyHookShareWrapper(KeyHook* key_hook)
    : key_hook_(key_hook) {
  DCHECK(key_hook_);
}

KeyHookShareWrapper::~KeyHookShareWrapper() = default;

void KeyHookShareWrapper::RegisterKey(
    const std::vector<ui::KeyboardCode>& codes,
    base::Callback<void(bool)> on_result) {
  key_hook_->RegisterKey(codes, std::move(on_result));
}

void KeyHookShareWrapper::UnregisterKey(
    const std::vector<ui::KeyboardCode>& codes,
    base::Callback<void(bool)> on_result) {
  key_hook_->UnregisterKey(codes, std::move(on_result));
}

}  // namespace keyboard_lock
