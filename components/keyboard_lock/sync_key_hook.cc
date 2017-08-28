// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/sync_key_hook.h"

namespace keyboard_lock {

SyncKeyHook::SyncKeyHook() = default;
SyncKeyHook::~SyncKeyHook() = default;

void SyncKeyHook::RegisterKey(const std::vector<ui::KeyboardCode>& codes,
                              base::Callback<void(bool)> on_result) {
  bool result = RegisterKey(codes);
  if (on_result) {
    on_result.Run(result);
  }
}

void SyncKeyHook::UnregisterKey(const std::vector<ui::KeyboardCode>& codes,
                                base::Callback<void(bool)> on_result) {
  bool result = UnregisterKey(codes);
  if (on_result) {
    on_result.Run(result);
  }
}

}  // namespace keyboard_lock
