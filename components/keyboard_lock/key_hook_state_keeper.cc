// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/key_hook_state_keeper.h"

#include <utility>

#include "base/logging.h"

namespace keyboard_lock {

KeyHookStateKeeper::KeyHookStateKeeper(std::unique_ptr<KeyEventFilter> filter,
                                       std::unique_ptr<KeyHook> key_hook)
    : filter_(std::move(filter)),
      key_hook_(std::move(key_hook)),
      states_(256),
      active_(false) {
  DCHECK(filter_);
  DCHECK(key_hook_);
}

KeyHookStateKeeper::~KeyHookStateKeeper() {
  // Ensure all keyboard lock are correctly deactivated.
  Deactivate(base::Callback<void(bool)>());
}

bool KeyHookStateKeeper::OnKeyDown(ui::KeyboardCode code, int flags) {
  if (code < 0 || static_cast<size_t>(code) >= states_.size()) {
    return false;
  }
  LOG(ERROR) << "Received OnKeyDown " << code << " - " << (states_[code] ? "Accept" : "Reject");
  return states_[code] && filter_->OnKeyDown(code, flags);
}

bool KeyHookStateKeeper::OnKeyUp(ui::KeyboardCode code, int flags) {
  if (code < 0 || static_cast<size_t>(code) >= states_.size()) {
    return false;
  }
  LOG(ERROR) << "Received OnKeyUp " << code << " - " << (states_[code] ? "Accept" : "Reject");
  return states_[code] && filter_->OnKeyUp(code, flags);
}

void KeyHookStateKeeper::RegisterKey(const std::vector<ui::KeyboardCode>& codes,
                                     base::Callback<void(bool)> on_result) {
  if (active_) {
    // Registers unregisted keyboard codes.
    std::vector<ui::KeyboardCode> unregistered_codes;
    for (const ui::KeyboardCode code : codes) {
      if (!states_[code]) {
        unregistered_codes.push_back(code);
        states_[code] = true;
        LOG(ERROR) << "Enable " << code;
      }
    }

    if (unregistered_codes.empty()) {
      if (on_result) {
        on_result.Run(true);
      }
    } else {
      key_hook_->RegisterKey(unregistered_codes, on_result);
    }
    return;
  }

  for (const ui::KeyboardCode code : codes) {
    int theCode = code;
    if (theCode == 65307) {
      theCode = 0x1b;
    }
    states_[theCode] = true;
  }
  if (on_result) {
    on_result.Run(true);
  }
}

void KeyHookStateKeeper::UnregisterKey(
    const std::vector<ui::KeyboardCode>& codes,
    base::Callback<void(bool)> on_result) {
  if (active_) {
    // Unregistes registered keyboard codes.
    std::vector<ui::KeyboardCode> registered_codes;
    for (const ui::KeyboardCode code : codes) {
      if (states_[code]) {
        registered_codes.push_back(code);
        states_[code] = false;
        LOG(ERROR) << "Disable " << code;
      }
    }

    if (registered_codes.empty()) {
      if (on_result) {
        on_result.Run(true);
      }
    } else {
      key_hook_->UnregisterKey(registered_codes, on_result);
    }
    return;
  }

  for (const ui::KeyboardCode code : codes) {
    states_[code] = false;
  }
  if (on_result) {
    on_result.Run(true);
  }
}

void KeyHookStateKeeper::Activate(base::Callback<void(bool)> on_result) {
  if (active_) {
    if (on_result) {
      on_result.Run(true);
    }
    return;
  }

  std::vector<ui::KeyboardCode> registered_codes;
  for (size_t i = 0; i < states_.size(); i++) {
    if (states_[i]) {
      registered_codes.push_back(static_cast<ui::KeyboardCode>(i));
    }
  }

  if (registered_codes.empty()) {
    if (on_result) {
      on_result.Run(true);
    }
  } else {
    key_hook_->RegisterKey(registered_codes, on_result);
  }
  active_ = true;
}

void KeyHookStateKeeper::Deactivate(base::Callback<void(bool)> on_result) {
  if (!active_) {
    if (on_result) {
      on_result.Run(true);
    }
    return;
  }

  std::vector<ui::KeyboardCode> registered_codes;
  for (size_t i = 0; i < states_.size(); i++) {
    if (states_[i]) {
      registered_codes.push_back(static_cast<ui::KeyboardCode>(i));
    }
  }

  if (registered_codes.empty()) {
    if (on_result) {
      on_result.Run(true);
    }
  } else {
    key_hook_->UnregisterKey(registered_codes, on_result);
  }
  active_ = false;
}

}  // namespace keyboard_lock
