// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_ACTIVE_KEY_EVENT_FILTER_REGISTRAR_H_
#define COMPONENTS_KEYBOARD_LOCK_ACTIVE_KEY_EVENT_FILTER_REGISTRAR_H_

#include <memory>

#include "components/keyboard_lock/key_event_filter.h"
#include "components/keyboard_lock/key_hook_activator.h"

namespace keyboard_lock {

class ActiveKeyEventFilterTracker;

// An implementation of KeyHookActivator to forward requests to |key_hook| and
// register / unregister cooresponding key event filter |filter| to |tracker|.
class ActiveKeyEventFilterRegistrar final : public KeyHookActivator {
 public:
  // |tracker| should outlive |this|.
  ActiveKeyEventFilterRegistrar(ActiveKeyEventFilterTracker* tracker,
                                std::unique_ptr<KeyHookActivator> key_hook,
                                KeyEventFilter* filter);
  ~ActiveKeyEventFilterRegistrar() override;

 private:
  // KeyHookActivator implementations
  void RegisterKey(const std::vector<ui::KeyboardCode>& codes,
                   base::Callback<void(bool)> on_result) override;
  void UnregisterKey(const std::vector<ui::KeyboardCode>& codes,
                     base::Callback<void(bool)> on_result) override;
  void Activate(base::Callback<void(bool)> on_result) override;
  void Deactivate(base::Callback<void(bool)> on_result) override;

  ActiveKeyEventFilterTracker* const tracker_;
  std::unique_ptr<KeyHookActivator> key_hook_;
  KeyEventFilter* const filter_;
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_ACTIVE_KEY_EVENT_FILTER_REGISTRAR_H_
