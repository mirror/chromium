// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/keyboard_lock/state_keeper.h"

#include <utility>

#include "ui/events/keycodes/dom/keycode_converter.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

StateKeeper::StateKeeper(std::unique_ptr<Client> client,
                         PlatformHook* platform_hook)
    : client_(std::move(client)),
      platform_hook_(platform_hook),
      states_(256),
      active_(false) {
  DCHECK(client_);
  DCHECK(platform_hook_);
}

StateKeeper::~StateKeeper() = default;

bool StateKeeper::OnKeyEvent(const ui::KeyEvent& event) {
  if (event.is_char()) {
    return false;
  }
  NativeKeyCode code = KeycodeConverter::DomCodeToNativeKeycode(event.code());
  if (!IsValidCode(code)) {
    return false;
  }

  if (states_[code]) {
    client_->OnKeyEvent(event);
    return true;
  }
  return false;
}

void StateKeeper::Register(const std::vector<NativeKeyCode>& codes,
                           base::Callback<void(bool)> on_result) {
  if (active_) {
    // Registers unregistered keyboard codes.
    std::vector<NativeKeyCode> unregistered_codes;
    for (const NativeKeyCode& code : codes) {
      if (IsValidCode(code) && !states_[code]) {
        unregistered_codes.push_back(code);
        states_[code] = true;
      }
    }

    if (unregistered_codes.empty()) {
      if (on_result) {
        on_result.Run(true);
      }
    } else {
      platform_hook_->Register(unregistered_codes, on_result);
    }
    return;
  }

  // if (!active_)
  for (const NativeKeyCode& code : codes) {
    if (IsValidCode(code)) {
      states_[code] = true;
    }
  }
  if (on_result) {
    on_result.Run(true);
  }
}

void StateKeeper::Unregister(const std::vector<NativeKeyCode>& codes,
                             base::Callback<void(bool)> on_result) {
  if (active_) {
    // Unregisters registered keyboard codes.
    std::vector<NativeKeyCode> registered_codes;
    for (const NativeKeyCode& code : codes) {
      if (IsValidCode(code) && states_[code]) {
        registered_codes.push_back(code);
        states_[code] = false;
      }
    }

    if (registered_codes.empty()) {
      if (on_result) {
        on_result.Run(true);
      }
    } else {
      platform_hook_->Unregister(registered_codes, on_result);
    }
    return;
  }

  // if (!active_)
  for (const NativeKeyCode& code : codes) {
    if (IsValidCode(code)) {
      states_[code] = false;
    }
  }
  if (on_result) {
    on_result.Run(true);
  }
}

void StateKeeper::Activate(base::Callback<void(bool)> on_result) {
  if (active_) {
    if (on_result) {
      on_result.Run(true);
    }
    return;
  }

  std::vector<NativeKeyCode> registered_codes;
  for (size_t i = 0; i < states_.size(); i++) {
    if (states_[i]) {
      registered_codes.push_back(static_cast<NativeKeyCode>(i));
    }
  }

  if (registered_codes.empty()) {
    if (on_result) {
      on_result.Run(true);
    }
  } else {
    platform_hook_->Register(registered_codes, on_result);
  }
  active_ = true;
}

void StateKeeper::Deactivate(base::Callback<void(bool)> on_result) {
  if (!active_) {
    if (on_result) {
      on_result.Run(true);
    }
    return;
  }

  std::vector<NativeKeyCode> registered_codes;
  for (size_t i = 0; i < states_.size(); i++) {
    if (states_[i]) {
      registered_codes.push_back(static_cast<NativeKeyCode>(i));
    }
  }

  if (registered_codes.empty()) {
    if (on_result) {
      on_result.Run(true);
    }
  } else {
    platform_hook_->Unregister(registered_codes, on_result);
  }
  active_ = false;
}

bool StateKeeper::IsValidCode(NativeKeyCode code) const {
  return code >= 0 && code < static_cast<int>(states_.size());
}

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui
