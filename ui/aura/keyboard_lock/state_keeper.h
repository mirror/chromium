// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_KEYBOARD_LOCK_STATE_KEEPER_H_
#define UI_AURA_KEYBOARD_LOCK_STATE_KEEPER_H_

#include <memory>
#include <vector>

#include "ui/aura/keyboard_lock/activator.h"
#include "ui/aura/keyboard_lock/client.h"
#include "ui/aura/keyboard_lock/platform_hook.h"
#include "ui/aura/keyboard_lock/types.h"
#include "ui/events/event.h"
#include "ui/events/platform/key_event_filter.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

// StateKeeper remembers the registered keys, so it supports OnKeyEvent() and
// Activate() / Deactivate() implementations. It forwards the Register() /
// Unregister() / Activate() and Deactivate() to |platform_hook_| and keyboard
// events to |client_|.
class StateKeeper final
    : public KeyEventFilter,
      public Activator {
 public:
  StateKeeper(std::unique_ptr<Client> client,
              PlatformHook* platform_hook);
  ~StateKeeper() override;

  bool OnKeyEvent(const ui::KeyEvent& event) override;

  void Register(const std::vector<NativeKeyCode>& codes,
                base::Callback<void(bool)> on_result) override;
  void Unregister(const std::vector<NativeKeyCode>& codes,
                  base::Callback<void(bool)> on_result) override;

  void Activate(base::Callback<void(bool)> on_result) override;
  void Deactivate(base::Callback<void(bool)> on_result) override;

 private:
  bool IsValidCode(NativeKeyCode code) const;

  const std::unique_ptr<Client> client_;
  PlatformHook* const platform_hook_;
  std::vector<bool> states_;
  bool active_;
};

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui

#endif  // UI_AURA_KEYBOARD_LOCK_STATE_KEEPER_H_
