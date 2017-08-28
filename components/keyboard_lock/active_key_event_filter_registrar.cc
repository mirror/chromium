// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/active_key_event_filter_registrar.h"

#include <utility>

#include "components/keyboard_lock/active_key_event_filter_tracker.h"

namespace keyboard_lock {

ActiveKeyEventFilterRegistrar::ActiveKeyEventFilterRegistrar(
    ActiveKeyEventFilterTracker* tracker,
    std::unique_ptr<KeyHookActivator> key_hook,
    KeyEventFilter* filter)
    : tracker_(tracker),
      key_hook_(std::move(key_hook)),
      filter_(filter) {
  DCHECK(tracker_);
  DCHECK(key_hook_);
  DCHECK(filter_);
}

ActiveKeyEventFilterRegistrar::~ActiveKeyEventFilterRegistrar() {
  tracker_->erase(filter_);
}

void ActiveKeyEventFilterRegistrar::RegisterKey(
    const std::vector<ui::KeyboardCode>& codes,
    base::Callback<void(bool)> on_result) {
  key_hook_->RegisterKey(codes, std::move(on_result));
}

void ActiveKeyEventFilterRegistrar::UnregisterKey(
    const std::vector<ui::KeyboardCode>& codes,
    base::Callback<void(bool)> on_result) {
  key_hook_->RegisterKey(codes, std::move(on_result));
}

void ActiveKeyEventFilterRegistrar::Activate(
    base::Callback<void(bool)> on_result) {
  key_hook_->Activate(std::move(on_result));
  tracker_->set(filter_);
}

void ActiveKeyEventFilterRegistrar::Deactivate(
    base::Callback<void(bool)> on_result) {
  key_hook_->Deactivate(std::move(on_result));
  tracker_->erase(filter_);
}

bool ActiveKeyEventFilterRegistrar::IsKeyReserved(ui::KeyboardCode code) const {
  return key_hook_->IsKeyReserved(code);
}

}  // namespace keyboard_lock
