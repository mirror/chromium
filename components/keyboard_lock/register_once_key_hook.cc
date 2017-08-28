// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/register_once_key_hook.h"

namespace keyboard_lock {

RegisterOnceKeyHook::RegisterOnceKeyHook() = default;
RegisterOnceKeyHook::~RegisterOnceKeyHook() = default;

bool RegisterOnceKeyHook::RegisterKey(
    const std::vector<ui::KeyboardCode>& codes) {
  bool result = false;
  if (count_ == 0) {
    result = Register();
  } else {
    result = true;
  }
  count_ += codes.size();
  return result;
}

bool RegisterOnceKeyHook::UnregisterKey(
    const std::vector<ui::KeyboardCode>& codes) {
  bool result = false;
  if (count_ <= static_cast<int>(codes.size())) {
    result = Unregister();
    count_ = 0;
  } else {
    result = true;
    count_ -= codes.size();
  }
  return result;
}

}  // namespace keyboard_lock
