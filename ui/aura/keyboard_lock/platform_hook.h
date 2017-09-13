// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_KEYBOARD_LOCK_PLATFORM_HOOK_H_
#define UI_AURA_KEYBOARD_LOCK_PLATFORM_HOOK_H_

#include "ui/aura/keyboard_lock/hook.h"
#include "ui/events/platform/key_event_filter.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

// PlatformHook is a Hook, but it also provides a KeyEventFilter to let platform
// dependent implementations to decide whether the keyboard event should be
// consumed.
class PlatformHook : public Hook {
 public:
  PlatformHook(KeyEventFilter* filter);
  ~PlatformHook() override;

  KeyEventFilter* filter() const;

 private:
  KeyEventFilter* const filter_;
};

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui

#endif  // UI_AURA_KEYBOARD_LOCK_PLATFORM_HOOK_H_
